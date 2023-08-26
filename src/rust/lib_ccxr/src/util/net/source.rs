use std::io;
use std::io::{Read, Write};
use std::net::{
    IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, SocketAddrV4, TcpListener, TcpStream, ToSocketAddrs,
    UdpSocket,
};

use socket2::{Domain, Socket, Type};

use crate::util::log::{fatal, info, ExitCause};
use crate::util::net::{Block, BlockStream, Command, NetError, DEFAULT_TCP_PORT, PING_INTERVAL};
use crate::util::time::Timestamp;

/// An enum of configuration parameters to construct [`RecvSource`].
#[derive(Copy, Clone, Debug)]
pub enum RecvSourceConfig<'a> {
    Tcp {
        /// The port number where TCP socket will be bound.
        ///
        /// If no port number is provided then [`DEFAULT_TCP_PORT`] will be used instead.
        port: Option<u16>,

        /// The password of receiving server.
        ///
        /// The password sent from client will be compared to this and only allow furthur
        /// communication if the passwords match.
        password: Option<&'a str>,
    },
    Udp {
        /// Source's IP address or hostname if trying to open a multicast SSM channel.
        source: Option<&'a str>,

        /// The IP address or hostname where UDP socket will be bound.
        address: Option<&'a str>,

        /// The port number where UDP socket will be bound.
        port: u16,
    },
}

enum SourceSocket {
    Tcp(TcpStream),
    Udp {
        socket: UdpSocket,
        source: Option<Ipv4Addr>,
        address: Ipv4Addr,
    },
}

/// A struct used for receiving subtitle data from the network.
///
/// Even though it exposes methods from [`BlockStream`], it is recommended to not use them.
/// Instead use the methods provided directly by [`RecvSource`] like
/// [`RecvSource::recv_header_or_cc`].
///
/// To create a [`RecvSource`], one must first construct a [`RecvSourceConfig`].
///
/// ```no_run
/// # use lib_ccxr::util::net::{RecvSource, RecvSourceConfig};
/// let config = RecvSourceConfig::Tcp {
///     port: None,
///     password: Some("12345678"),
/// };
/// let mut recv_source = RecvSource::new(config);
///
/// // Once recv_source is constructed, we can use it to receive data.
/// let block = recv_source.recv_header_or_cc().unwrap();
/// ```
pub struct RecvSource {
    socket: SourceSocket,
    last_ping: Timestamp,
}

impl BlockStream for RecvSource {
    fn send(&mut self, buf: &[u8]) -> io::Result<usize> {
        match &mut self.socket {
            SourceSocket::Tcp(stream) => stream.write(buf),
            SourceSocket::Udp { socket, .. } => socket.send(buf),
        }
    }

    fn recv(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        match &mut self.socket {
            SourceSocket::Tcp(stream) => stream.read(buf),
            SourceSocket::Udp {
                socket,
                source,
                address,
            } => {
                if cfg!(target = "windows") {
                    socket.recv(buf)
                } else {
                    let should_check_source = address.is_multicast() && source.is_some();
                    if should_check_source {
                        loop {
                            if let Ok((size, src_addr)) = socket.recv_from(buf) {
                                if src_addr.ip() == source.unwrap() {
                                    return Ok(size);
                                }
                            }
                        }
                    } else {
                        socket.recv(buf)
                    }
                }
            }
        }
    }
}

impl RecvSource {
    /// Create a new [`RecvSource`] from the configuration parameters of [`RecvSourceConfig`].
    ///
    /// Note that this method attempts to create a server. It does not return a [`Result`]. When
    /// it is unable to start a server, it crashes instantly by calling [`fatal!`].
    ///
    /// This method will continously block until a connection with a client is made.
    pub fn new(config: RecvSourceConfig) -> RecvSource {
        match config {
            RecvSourceConfig::Tcp { port, password } => {
                let port = port.unwrap_or(DEFAULT_TCP_PORT);

                info!(
                    "\n\r----------------------------------------------------------------------\n"
                );
                info!("Binding to {}\n", port);

                let addresses = [
                    SocketAddr::new(IpAddr::V6(Ipv6Addr::UNSPECIFIED), port),
                    SocketAddr::new(IpAddr::V4(Ipv4Addr::UNSPECIFIED), port),
                ];

                let listener = TcpListener::bind(addresses.as_slice()).unwrap_or_else(
                    |_| fatal!(cause = ExitCause::Failure; "Unable to start server\n"),
                );

                if let Some(pwd) = &password {
                    info!("Password: {}\n", pwd);
                }

                info!("Waiting for connections\n");

                loop {
                    if let Ok((socket, _)) = listener.accept() {
                        let mut source = RecvSource {
                            socket: SourceSocket::Tcp(socket),
                            last_ping: Timestamp::from_millis(0),
                        };
                        if check_password(&mut source, password) {
                            return source;
                        }
                    }
                }
            }
            RecvSourceConfig::Udp {
                source,
                address,
                port,
            } => {
                let address = address
                    .map(|x| {
                        for s in (x, 0).to_socket_addrs().unwrap() {
                            if let SocketAddr::V4(sv4) = s {
                                return *sv4.ip();
                            }
                        }
                        fatal!(cause = ExitCause::Failure; "Could not resolve udp address")
                    })
                    .unwrap_or(Ipv4Addr::UNSPECIFIED);

                let source = source.map(|x| {
                    for s in (x, 0).to_socket_addrs().unwrap() {
                        if let SocketAddr::V4(sv4) = s {
                            return *sv4.ip();
                        }
                    }
                    fatal!(cause = ExitCause::Failure; "Could not resolve udp source")
                });

                let socket = Socket::new(Domain::IPV4, Type::DGRAM, None).unwrap_or_else(
                    |_| fatal!(cause = ExitCause::Failure; "Socket creation error"),
                );

                if address.is_multicast() {
                    socket.set_reuse_address(true).unwrap_or_else(|_| {
                        info!("Cannot not set reuse address\n");
                    });
                }

                let binding_address = if cfg!(target_os = "windows") && address.is_multicast() {
                    Ipv4Addr::UNSPECIFIED
                } else {
                    address
                };

                socket
                    .bind(&SocketAddrV4::new(binding_address, port).into())
                    .unwrap_or_else(|_| fatal!(cause = ExitCause::Bug; "Socket bind error"));

                if address.is_multicast() {
                    if let Some(src) = source {
                        socket.join_ssm_v4(&src, &address, &Ipv4Addr::UNSPECIFIED)
                    } else {
                        socket.join_multicast_v4(&address, &Ipv4Addr::UNSPECIFIED)
                    }
                    .unwrap_or_else(
                        |_| fatal!(cause = ExitCause::Bug; "Cannot join multicast group"),
                    );
                }

                info!(
                    "\n\r----------------------------------------------------------------------\n"
                );
                if address == Ipv4Addr::UNSPECIFIED {
                    info!("\rReading from UDP socket {}\n", port);
                } else if let Some(src) = source {
                    info!("\rReading from UDP socket {}@{}:{}\n", src, address, port);
                } else {
                    info!("\rReading from UDP socket {}:{}\n", address, port);
                }

                RecvSource {
                    socket: SourceSocket::Udp {
                        socket: socket.into(),
                        address,
                        source,
                    },
                    last_ping: Timestamp::from_millis(0),
                }
            }
        }
    }

    /// Receive a [`BinHeader`] or [`BinData`] [`Block`].
    ///
    /// Note that this method will continously block until it receives a
    /// [`BinHeader`] or [`BinData`] [`Block`].
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred or byte format is violated.
    /// It will return a [`None`] if the connection has shutdown down correctly.
    ///
    /// [`BinHeader`]: Command::BinHeader
    /// [`BinData`]: Command::BinData
    pub fn recv_header_or_cc<'a>(&mut self) -> Result<Option<Block<'a>>, NetError> {
        let now = Timestamp::now();
        if self.last_ping.millis() == 0 {
            self.last_ping = now;
        }

        if now - self.last_ping > PING_INTERVAL {
            self.last_ping = now;
            if self.send_ping().is_err() {
                fatal!(cause = ExitCause::Failure; "Unable to send keep-alive packet to client\n");
            }
        }

        loop {
            if let Some(block) = self.recv_block()? {
                if block.command() == Command::BinHeader || block.command() == Command::BinData {
                    return Ok(Some(block));
                }
            } else {
                return Ok(None);
            }
        }
    }

    /// Send a [`Ping`](Command::Ping) [`Block`].
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred, or else it will return a bool
    /// that indicates the status of this connection. It will be `false` if the connection shutdown
    /// correctly.
    fn send_ping(&mut self) -> Result<bool, NetError> {
        self.send_block(&Block::ping())
    }
}

/// Check if the received password matches with the current password.
///
/// This methods attempts to read a [`Password`](Command::Password) [`Block`] from `socket`. Any
/// form of error in this operation results in `false`.
///
/// If `password` is [`None`], then no checking is done and results in `true`.
fn check_password(socket: &mut RecvSource, password: Option<&str>) -> bool {
    let block = match socket.recv_block() {
        Ok(Some(b)) => b,
        _ => return false,
    };

    let pwd = match password {
        Some(p) => p,
        None => return true,
    };

    if block.command() == Command::Password && String::from_utf8_lossy(block.data()) == *pwd {
        true
    } else {
        #[cfg(feature = "debug_out")]
        {
            eprintln!("[C] Wrong password");
            eprintln!("[S] PASSWORD");
        }
        // TODO: Check if the below portion is really required, since the receiver is not even
        // listening for a Password block
        let _ = socket.send_block(&Block::password(""));

        false
    }
}
