use crate::util::log::{fatal, info, ExitCause};
use crate::util::net::{
    Block, BlockStream, Command, NetError, DEFAULT_TCP_PORT, NO_RESPONSE_INTERVAL, PING_INTERVAL,
};
use crate::util::time::Timestamp;
use std::io;
use std::io::{Read, Write};
use std::net::TcpStream;

/// A struct of configuration parameters to construct [`SendTarget`].
#[derive(Copy, Clone, Debug)]
pub struct SendTargetConfig<'a> {
    /// Target's IP address or hostname.
    pub target_addr: &'a str,

    /// Target's port number.
    ///
    /// If no port number is provided then [`DEFAULT_TCP_PORT`] will be used instead.
    pub port: Option<u16>,

    /// Password to be sent after establishing connection.
    pub password: Option<&'a str>,

    /// Description to sent after establishing connection.
    pub description: Option<&'a str>,
}

/// A struct used for sending subtitle data across the network.
///
/// Even though it exposes methods from [`BlockStream`], it is recommended to not use them.
/// Instead use the methods provided directly by [`SendTarget`] like [`SendTarget::send_header`],
/// [`SendTarget::send_cc`], etc.
///
/// To create a [`SendTarget`], one must first construct a [`SendTargetConfig`].
///
/// ```no_run
/// # use lib_ccxr::util::net::{SendTarget, SendTargetConfig};
/// let config = SendTargetConfig {
///     target_addr: "192.168.60.133",
///     port: None,
///     password: Some("12345678"),
///     description: None,
/// };
/// let mut send_target = SendTarget::new(config);
///
/// // Once send_target is constructed, we can use it to send different kinds of data.
/// # let header = &[0u8; 1];
/// send_target.send_header(header);
/// # let cc = &[0u8; 1];
/// send_target.send_cc(cc);
/// ```
pub struct SendTarget<'a> {
    stream: Option<TcpStream>,
    config: SendTargetConfig<'a>,
    header_data: Option<Vec<u8>>,
    last_ping: Timestamp,
    last_send_ping: Timestamp,
}

impl BlockStream for SendTarget<'_> {
    fn send(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.stream.as_mut().unwrap().write(buf)
    }

    fn recv(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.stream.as_mut().unwrap().read(buf)
    }
}

impl<'a> SendTarget<'a> {
    /// Create a new [`SendTarget`] from the configuration parameters of [`SendTargetConfig`].
    ///
    /// Note that this method attempts to connect to a server. It does not return a [`Result`]. When
    /// it is unable to connect to a server, it crashes instantly by calling [`fatal!`].
    pub fn new(config: SendTargetConfig<'a>) -> SendTarget<'a> {
        let tcp_stream = TcpStream::connect((
            config.target_addr,
            config.port.unwrap_or(DEFAULT_TCP_PORT),
        ))
        .unwrap_or_else(
            |_| fatal!(cause = ExitCause::Failure; "Unable to connect (tcp connection error)."),
        );

        tcp_stream.set_nonblocking(true).unwrap_or_else(
            |_| fatal!(cause = ExitCause::Failure; "Unable to connect (set nonblocking).\n"),
        );

        let mut send_target = SendTarget {
            stream: Some(tcp_stream),
            config,
            header_data: None,
            last_ping: Timestamp::from_millis(0),
            last_send_ping: Timestamp::from_millis(0),
        };

        send_target.send_password().unwrap_or_else(
            |_| fatal!(cause = ExitCause::Failure; "Unable to connect (sending password).\n"),
        );

        send_target.send_description().unwrap_or_else(
            |_| fatal!(cause = ExitCause::Failure; "Unable to connect (sending cc_desc).\n"),
        );

        info!(
            "Connected to {}\n",
            send_target.stream.as_ref().unwrap().peer_addr().unwrap()
        );

        send_target
    }

    /// Consumes the [`SendTarget`] only returning its internal stream.
    fn into_stream(self) -> TcpStream {
        self.stream.unwrap()
    }

    /// Send a [`BinHeader`](Command::BinHeader) [`Block`] returning if the operation was successful.
    pub fn send_header(&mut self, data: &[u8]) -> bool {
        #[cfg(feature = "debug_out")]
        {
            eprintln!("Sending header (len = {}): ", data.len());
            eprintln!(
                "File created by {:02X} version {:02X}{:02X}",
                data[3], data[4], data[5]
            );
            eprintln!("File format revision: {:02X}{:02X}", data[6], data[7]);
        }

        if let Ok(true) = self.send_block(&Block::bin_header(data)) {
        } else {
            println!("Can't send BIN header");
            return false;
        }

        if self.header_data.is_none() {
            self.header_data = Some(data.into())
        }

        true
    }

    /// Send a [`BinData`](Command::BinData) [`Block`] returning if the operation was successful.
    pub fn send_cc(&mut self, data: &[u8]) -> bool {
        #[cfg(feature = "debug_out")]
        {
            eprintln!("[C] Sending {} bytes", data.len());
        }

        if let Ok(true) = self.send_block(&Block::bin_data(data)) {
        } else {
            println!("Can't send BIN data");
            return false;
        }

        true
    }

    /// Send a [`EpgData`](Command::EpgData) [`Block`] returning if the operation was successful.
    pub fn send_epg_data(
        &mut self,
        start: &str,
        stop: &str,
        title: Option<&str>,
        desc: Option<&str>,
        lang: Option<&str>,
        category: Option<&str>,
    ) -> bool {
        let block = Block::epg_data(start, stop, title, desc, lang, category);

        #[cfg(feature = "debug_out")]
        {
            eprintln!("[C] Sending EPG: {} bytes", block.data().len())
        }

        if let Ok(true) = self.send_block(&block) {
        } else {
            eprintln!("Can't send EPG data");
            return false;
        }

        true
    }

    /// Send a [`Ping`](Command::Ping) [`Block`].
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred, or else it will return a bool
    /// that indicates the status of this connection. It will be `false` if the connection shutdown
    /// correctly.
    fn send_ping(&mut self) -> Result<bool, NetError> {
        self.send_block(&Block::ping())
    }

    /// Send a [`Password`](Command::Password) [`Block`].
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred, or else it will return a bool
    /// that indicates the status of this connection. It will be `false` if the connection shutdown
    /// correctly.
    fn send_password(&mut self) -> Result<bool, NetError> {
        let password = self.config.password.unwrap_or("");
        self.send_block(&Block::password(password))
    }

    /// Send a [`CcDesc`](Command::CcDesc) [`Block`].
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred, or else it will return a bool
    /// that indicates the status of this connection. It will be `false` if the connection shutdown
    /// correctly.
    fn send_description(&mut self) -> Result<bool, NetError> {
        let description = self.config.description.unwrap_or("");
        self.send_block(&Block::cc_desc(description))
    }

    /// Check the connection health and reset connection if necessary.
    ///
    /// This method determines the connection health by comparing the time since last [`Ping`]
    /// [`Block`] was received with [`NO_RESPONSE_INTERVAL`]. If it exceeds the
    /// [`NO_RESPONSE_INTERVAL`], the connection is reset.
    ///
    /// This method also sends timely [`Ping`] [`Block`]s back to the server based on the
    /// [`PING_INTERVAL`]. This method will crash instantly with [`fatal!`] if it is unable to send
    /// data.
    ///
    /// [`Ping`]: Command::Ping
    pub fn check_connection(&mut self) {
        let now = Timestamp::now();

        if self.last_ping.millis() == 0 {
            self.last_ping = now;
        }

        loop {
            if self
                .recv_block()
                .ok()
                .flatten()
                .map(|x| x.command() == Command::Ping)
                .unwrap_or(false)
            {
                #[cfg(feature = "debug_out")]
                {
                    eprintln!("[S] Received PING");
                }
                self.last_ping = now;
            } else {
                break;
            }
        }

        if now - self.last_ping > NO_RESPONSE_INTERVAL {
            eprintln!(
                "[S] No PING received from the server in {} sec, reconnecting",
                NO_RESPONSE_INTERVAL.seconds()
            );

            std::mem::drop(self.stream.take().unwrap());

            self.stream = Some(SendTarget::new(self.config).into_stream());

            // `self.header_data` is only temporarily taken, since it will be refilled inside
            // `send_header` function.
            if let Some(header_data) = self.header_data.take() {
                self.send_header(header_data.as_slice());
            }

            self.last_ping = now;
        }

        if now - self.last_send_ping >= PING_INTERVAL {
            if self.send_ping().is_err() {
                fatal!(cause = ExitCause::Failure; "Unable to send data\n");
            }

            self.last_send_ping = now;
        }
    }
}
