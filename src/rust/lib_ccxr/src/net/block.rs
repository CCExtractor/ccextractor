use super::NetError;
use crate::time::units::Timestamp;

use num_enum::{IntoPrimitive, TryFromPrimitive};

use std::borrow::Cow;
use std::fmt::{self, Display, Formatter};
use std::io::{self, Write};

/// Default port to be used when port number is not specified for TCP.
pub const DEFAULT_TCP_PORT: u16 = 2048;

/// The amount of time to wait for a response before reseting the connection.
pub const NO_RESPONSE_INTERVAL: Timestamp = Timestamp::from_millis(20_000);

/// The time interval between sending ping messages.
pub const PING_INTERVAL: Timestamp = Timestamp::from_millis(3_000);

/// The size of the `length` section of the [`Block`]'s byte format.
///
/// See [`BlockStream`] for more information.
pub const LEN_SIZE: usize = 10;

/// The sequence of bytes used to denote the end of a [`Block`] in its byte format.
///
/// See [`BlockStream`] for more information.
pub const END_MARKER: &str = "\r\n";

/// Represents the different kinds of commands that could be sent or received in a block.
#[derive(Copy, Clone, Debug, Eq, PartialEq, IntoPrimitive, TryFromPrimitive)]
#[repr(u8)]
pub enum Command {
    Ok = 1,

    /// Used to send password just after making a TCP connection.
    Password = 2,
    BinMode = 3,

    /// Used to send description just after making a TCP connection.
    CcDesc = 4,
    BinHeader = 5,
    BinData = 6,

    /// Used to send EPG metadata across network.
    EpgData = 7,
    Error = 51,
    UnknownCommand = 52,
    WrongPassword = 53,
    ConnLimit = 54,

    /// Used to send ping messages to check network connectivity.
    Ping = 55,
}

/// Represents the smallest unit of data that could be sent or received from network.
///
/// A [`Block`] consists of a [`Command`] and some binary data associated with it. The kind of
/// block is denoted by its [`Command`]. The binary data has different formats or information based
/// on the kind of [`Command`].
///
/// Any subtitle data, metadata, ping, etc, that needs to be sent by network goes through in the
/// form of a [`Block`]. See [`BlockStream`] for more information on how a [`Block`] is sent or
/// received.
pub struct Block<'a> {
    command: Command,
    data: Cow<'a, [u8]>,
}

impl<'a> Block<'a> {
    fn new(command: Command, data: &'a [u8]) -> Block<'a> {
        Block {
            command,
            data: Cow::from(data),
        }
    }

    fn new_owned(command: Command, data: Vec<u8>) -> Block<'a> {
        Block {
            command,
            data: Cow::from(data),
        }
    }

    /// Returns the kind of [`Block`] denoted by its [`Command`].
    pub fn command(&self) -> Command {
        self.command
    }

    /// Returns the associated data of [`Block`].
    pub fn data(&self) -> &[u8] {
        &self.data
    }

    /// Create a new [`Ping`](Command::Ping) Block.
    pub fn ping() -> Block<'a> {
        Block::new_owned(Command::Ping, vec![])
    }

    /// Create a new [`BinHeader`](Command::BinHeader) Block along with `header` data.
    pub fn bin_header(header: &'a [u8]) -> Block<'a> {
        Block::new(Command::BinHeader, header)
    }

    /// Create a new [`BinData`](Command::BinData) Block along with `data`.
    pub fn bin_data(data: &'a [u8]) -> Block<'a> {
        Block::new(Command::BinData, data)
    }

    /// Create a new [`Password`](Command::Password) Block along with `password` data.
    ///
    /// The data of the returned [`Block`] will consist of `password` encoded as UTF-8 bytes which  
    /// is not nul-terminated.
    ///
    /// # Examples
    /// ```
    /// # use lib_ccxr::net::Block;
    /// let b = Block::password("A");
    /// assert_eq!(b.data(), &[b'A']);
    /// ```
    pub fn password(password: &'a str) -> Block<'a> {
        Block::new(Command::Password, password.as_bytes())
    }

    /// Create a new [`CcDesc`](Command::CcDesc) Block along with `desc` data.
    ///
    /// The data of the returned [`Block`] will consist of `desc` encoded as UTF-8 bytes which is
    /// not nul-terminated.
    ///
    /// # Examples
    /// ```
    /// # use lib_ccxr::net::Block;
    /// let b = Block::cc_desc("Teletext");
    /// assert_eq!(b.data(), &[b'T', b'e', b'l', b'e', b't', b'e', b'x', b't']);
    /// ```
    pub fn cc_desc(desc: &'a str) -> Block<'a> {
        Block::new(Command::CcDesc, desc.as_bytes())
    }

    /// Create a new [`EpgData`](Command::EpgData) Block along with the related metadata used in
    /// EPG.
    ///
    /// All the parameters are encoded as UTF-8 bytes which are nul-terminated. If a parameter is
    /// [`None`], then it is considered to be equivalent to an empty String. All these
    /// nul-terminated UTF-8 bytes are placed one after the other in the order of `start`, `stop`,
    /// `title`, `desc`, `lang`, `category` with nul character acting as the seperator between
    /// these sections.
    ///
    /// # Examples
    /// ```
    /// # use lib_ccxr::net::Block;
    /// let b = Block::epg_data("A", "B", Some("C"), None, Some("D"), None);
    /// assert_eq!(b.data(), &[b'A', b'\0', b'B', b'\0', b'C', b'\0', b'\0', b'D', b'\0', b'\0']);
    /// ```
    pub fn epg_data(
        start: &str,
        stop: &str,
        title: Option<&str>,
        desc: Option<&str>,
        lang: Option<&str>,
        category: Option<&str>,
    ) -> Block<'a> {
        let title = title.unwrap_or("");
        let desc = desc.unwrap_or("");
        let lang = lang.unwrap_or("");
        let category = category.unwrap_or("");

        // Plus 1 to accomodate space for the nul character
        let start_len = start.len() + 1;
        let stop_len = stop.len() + 1;
        let title_len = title.len() + 1;
        let desc_len = desc.len() + 1;
        let lang_len = lang.len() + 1;
        let category_len = category.len() + 1;

        let total_len = start_len + stop_len + title_len + desc_len + lang_len + category_len;
        let mut data = Vec::with_capacity(total_len);

        data.extend_from_slice(start.as_bytes());
        data.extend_from_slice("\0".as_bytes());
        data.extend_from_slice(stop.as_bytes());
        data.extend_from_slice("\0".as_bytes());
        data.extend_from_slice(title.as_bytes());
        data.extend_from_slice("\0".as_bytes());
        data.extend_from_slice(desc.as_bytes());
        data.extend_from_slice("\0".as_bytes());
        data.extend_from_slice(lang.as_bytes());
        data.extend_from_slice("\0".as_bytes());
        data.extend_from_slice(category.as_bytes());
        data.extend_from_slice("\0".as_bytes());

        Block::new_owned(Command::EpgData, data)
    }
}

/// The [`BlockStream`] trait allows for sending and receiving [`Block`]s across the network.
///
/// The only two implementers of [`BlockStream`] are [`SendTarget`] and [`RecvSource`] which are
/// used for sending and receiving blocks respectively.
///
/// This trait provides an abstraction over the different interfaces of [`TcpStream`] and
/// [`UdpSocket`]. The implementers only need to implement the functionality to send and receive
/// bytes through network by [`BlockStream::send`] and [`BlockStream::recv`]. The functionality to
/// send and receive [`Block`] will be automatically made available by [`BlockStream::send_block`]
/// and [`BlockStream::recv_block`].
///
/// A [`Block`] is sent or received across the network using a byte format. Since a [`Block`]
/// consists of `command` and variable sized `data`, it is encoded in the following way.
///
/// | Section    | Length       | Description                                                               |
/// |------------|--------------|---------------------------------------------------------------------------|
/// | command    | 1            | The `command` enconded as its corresponding byte value.                   |
/// | length     | [`LEN_SIZE`] | The length of `data` encoded as nul-terminated string form of the number. |
/// | data       | length       | The associated `data` bytes whose meaning is dependent on `command`.      |
/// | end_marker | 2            | This value is always [`END_MARKER`], signifying end of current block.     |
///
/// The only exception to the above format is a [`Ping`] [`Block`] which is encoded only with its 1-byte
/// command section. It does not have length, data or end_marker sections.
///
/// [`SendTarget`]: super::SendTarget
/// [`RecvSource`]: super::RecvSource
/// [`TcpStream`]: std::net::TcpStream
/// [`UdpSocket`]: std::net::UdpSocket
/// [`Ping`]: Command::Ping
pub trait BlockStream {
    /// Send the bytes in `buf` across the network.
    fn send(&mut self, buf: &[u8]) -> io::Result<usize>;

    /// Receive the bytes from network and place them in `buf`.
    fn recv(&mut self, buf: &mut [u8]) -> io::Result<usize>;

    /// Send all bytes in `buf` across the network.
    fn send_all(&mut self, mut buf: &[u8]) -> Result<bool, NetError> {
        while !buf.is_empty() {
            match self.send(buf) {
                Ok(0) => return Ok(false),
                Ok(sent) => buf = &buf[sent..],
                Err(error) if error.kind() == io::ErrorKind::Interrupted => {}
                Err(error) => return Err(error.into()),
            }
        }

        Ok(true)
    }

    /// Receive enough bytes from the network to fill `buf`.
    fn recv_exact(&mut self, mut buf: &mut [u8]) -> Result<bool, NetError> {
        while !buf.is_empty() {
            match self.recv(buf) {
                Ok(0) => return Ok(false),
                Ok(received) => buf = &mut buf[received..],
                Err(error) if error.kind() == io::ErrorKind::Interrupted => {}
                Err(error) => return Err(error.into()),
            }
        }

        Ok(true)
    }

    /// Receive and discard `length` bytes from the network.
    fn discard(&mut self, mut length: usize) -> Result<bool, NetError> {
        let mut buffer = [0_u8; 4096];

        while length > 0 {
            let chunk_length = length.min(buffer.len());
            if !self.recv_exact(&mut buffer[..chunk_length])? {
                return Ok(false);
            }
            length -= chunk_length;
        }

        Ok(true)
    }

    /// Send a [`Block`] across the network.
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred, or else it will return a bool
    /// that indicates the status of this connection. It will be `false` if the connection shutdown
    /// correctly.
    fn send_block(&mut self, block: &Block<'_>) -> Result<bool, NetError> {
        let Block { command, data } = block;

        if !self.send_all(&[(*command).into()])? {
            return Ok(false);
        }

        if *command == Command::Ping {
            return Ok(true);
        }

        let mut length_part = [0; LEN_SIZE];
        let _ = write!(length_part.as_mut_slice(), "{}", data.len());
        if !self.send_all(&length_part)? {
            return Ok(false);
        }

        if !self.send_all(data)? {
            return Ok(false);
        }

        if !self.send_all(END_MARKER.as_bytes())? {
            return Ok(false);
        }

        #[cfg(feature = "debug_out")]
        eprintln!("block = {}", block);

        Ok(true)
    }

    /// Receive a [`Block`] from the network.
    ///
    /// Data larger than `max_data_length` is discarded before returning an error so the next block
    /// remains readable.
    ///
    /// It returns a [`NetError`] if some transmission failure ocurred or byte format is violated.
    /// It will return a [`None`] if the connection has shutdown down correctly.
    fn recv_block<'a>(&mut self, max_data_length: usize) -> Result<Option<Block<'a>>, NetError> {
        let mut command_byte = [0_u8; 1];
        if !self.recv_exact(&mut command_byte)? {
            return Ok(None);
        }

        let command: Command = command_byte[0]
            .try_into()
            .map_err(|_| NetError::InvalidBytes {
                location: "command",
            })?;

        if command == Command::Ping {
            return Ok(Some(Block::ping()));
        }

        let mut length_bytes = [0u8; LEN_SIZE];
        if !self.recv_exact(&mut length_bytes)? {
            return Ok(None);
        }
        let end = length_bytes
            .iter()
            .position(|&x| x == b'\0')
            .unwrap_or(LEN_SIZE);
        let length: usize = String::from_utf8_lossy(&length_bytes[0..end])
            .parse()
            .map_err(|_| NetError::InvalidBytes { location: "length" })?;

        let data = if length <= max_data_length {
            let mut data = vec![0u8; length];
            if !self.recv_exact(&mut data)? {
                return Ok(None);
            }
            Some(data)
        } else {
            if !self.discard(length)? {
                return Ok(None);
            }
            None
        };

        let mut end_marker = [0u8; END_MARKER.len()];
        if !self.recv_exact(&mut end_marker)? {
            return Ok(None);
        }
        if end_marker != END_MARKER.as_bytes() {
            return Err(NetError::InvalidBytes {
                location: "end_marker",
            });
        }

        let data = data.ok_or(NetError::InvalidBytes { location: "length" })?;

        let block = Block::new_owned(command, data);

        #[cfg(feature = "debug_out")]
        eprintln!("{}", block);

        Ok(Some(block))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::VecDeque;

    struct TestStream {
        sent: Vec<u8>,
        received: VecDeque<u8>,
        max_send_length: usize,
        max_recv_length: usize,
        interrupt_send: bool,
        interrupt_recv: bool,
    }

    impl TestStream {
        fn new(received: Vec<u8>, max_send_length: usize, max_recv_length: usize) -> Self {
            Self {
                sent: Vec::new(),
                received: received.into(),
                max_send_length,
                max_recv_length,
                interrupt_send: false,
                interrupt_recv: false,
            }
        }
    }

    impl BlockStream for TestStream {
        fn send(&mut self, buf: &[u8]) -> io::Result<usize> {
            if self.interrupt_send {
                self.interrupt_send = false;
                return Err(io::ErrorKind::Interrupted.into());
            }
            let length = buf.len().min(self.max_send_length);
            self.sent.extend_from_slice(&buf[..length]);
            Ok(length)
        }

        fn recv(&mut self, buf: &mut [u8]) -> io::Result<usize> {
            if self.interrupt_recv {
                self.interrupt_recv = false;
                return Err(io::ErrorKind::Interrupted.into());
            }
            let length = buf.len().min(self.max_recv_length).min(self.received.len());
            for byte in &mut buf[..length] {
                *byte = self.received.pop_front().unwrap();
            }
            Ok(length)
        }
    }

    fn encoded_block(command: Command, data: &[u8]) -> Vec<u8> {
        let mut encoded = vec![command.into()];
        let mut length = [0_u8; LEN_SIZE];
        let _ = write!(length.as_mut_slice(), "{}", data.len());
        encoded.extend_from_slice(&length);
        encoded.extend_from_slice(data);
        encoded.extend_from_slice(END_MARKER.as_bytes());
        encoded
    }

    #[test]
    fn send_block_handles_partial_writes() {
        let mut stream = TestStream::new(Vec::new(), 2, 2);
        let block = Block::bin_data(b"caption");

        assert!(stream.send_block(&block).unwrap());
        assert_eq!(stream.sent, encoded_block(Command::BinData, b"caption"));
    }

    #[test]
    fn recv_block_handles_partial_reads() {
        let encoded = encoded_block(Command::BinData, b"caption");
        let mut stream = TestStream::new(encoded, 2, 2);

        let block = stream.recv_block(7).unwrap().unwrap();
        assert_eq!(block.command(), Command::BinData);
        assert_eq!(block.data(), b"caption");
    }

    #[test]
    fn block_stream_retries_interrupted_io() {
        let encoded = encoded_block(Command::BinData, b"caption");
        let mut stream = TestStream::new(encoded, 2, 2);
        stream.interrupt_send = true;
        stream.interrupt_recv = true;

        assert!(stream.send_block(&Block::bin_data(b"caption")).unwrap());
        let block = stream.recv_block(7).unwrap().unwrap();
        assert_eq!(block.data(), b"caption");
    }

    #[test]
    fn recv_block_returns_none_for_partial_frame_at_eof() {
        let mut encoded = encoded_block(Command::BinData, b"caption");
        encoded.pop();
        let mut stream = TestStream::new(encoded, 2, 2);

        assert!(stream.recv_block(7).unwrap().is_none());
    }

    #[test]
    fn recv_block_rejects_invalid_length() {
        let mut encoded = vec![Command::BinData.into()];
        encoded.extend_from_slice(b"invalid\0\0\0");
        let mut stream = TestStream::new(encoded, 2, 2);

        assert!(matches!(
            stream.recv_block(7),
            Err(NetError::InvalidBytes { location: "length" })
        ));
    }

    #[test]
    fn recv_block_discards_oversized_frame() {
        let mut encoded = encoded_block(Command::BinData, b"oversized");
        encoded.extend_from_slice(&encoded_block(Command::BinData, b"next"));
        let mut stream = TestStream::new(encoded, 2, 2);

        assert!(matches!(
            stream.recv_block(4),
            Err(NetError::InvalidBytes { location: "length" })
        ));

        let block = stream.recv_block(4).unwrap().unwrap();
        assert_eq!(block.command(), Command::BinData);
        assert_eq!(block.data(), b"next");
    }
}

impl Display for Block<'_> {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        let _ = write!(f, "[Rust] {} {} ", self.command, self.data.len());

        if self.command != Command::BinHeader && self.command != Command::BinData {
            let _ = write!(f, "{} ", &*String::from_utf8_lossy(&self.data));
        }

        let _ = write!(f, "\\r\\n");

        Ok(())
    }
}

impl Display for Command {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        let message = match self {
            Command::Ok => "OK",
            Command::Password => "PASSWORD",
            Command::BinMode => "BIN_MODE",
            Command::CcDesc => "CC_DESC",
            Command::BinHeader => "BIN_HEADER",
            Command::BinData => "BIN_DATA",
            Command::EpgData => "EPG_DATA",
            Command::Error => "ERROR",
            Command::UnknownCommand => "UNKNOWN_COMMAND",
            Command::WrongPassword => "WRONG_PASSWORD",
            Command::ConnLimit => "CONN_LIMIT",
            Command::Ping => "PING",
        };

        let _ = write!(f, "{message}");

        Ok(())
    }
}
