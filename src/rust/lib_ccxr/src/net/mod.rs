//! A module for sending and receiving subtitle data across the network.
//!
//! The [`SendTarget`] struct provides methods to send data to the network. It can be constructed
//! from [`SendTargetConfig`].
//!
//! The [`RecvSource`] struct provides methods to receive data from the network. It can be
//! constructed from [`RecvSourceConfig`].
//!
//! Any data to be sent across the network in the stored in the form of a [`Block`]. The
//! [`BlockStream`] can encode a [`Block`] as a byte sequence using a custom byte format which can
//! then be sent or received using standard networking primitives. See [`BlockStream`] to know more
//! about the custom byte format.
//!
//! # Conversion Guide
//!
//! | From                           | To                                |
//! |--------------------------------|-----------------------------------|
//! | `connect_to_srv`               | [`SendTarget::new`]               |
//! | `net_send_header`              | [`SendTarget::send_header`]       |
//! | `net_send_cc`                  | [`SendTarget::send_cc`]           |
//! | `net_send_epg`                 | [`SendTarget::send_epg_data`]     |
//! | `net_check_conn`               | [`SendTarget::check_connection`]  |
//! | `start_tcp_srv`                | [`RecvSource::new`]               |
//! | `start_udp_srv`                | [`RecvSource::new`]               |
//! | `net_tcp_read`                 | [`RecvSource::recv_header_or_cc`] |
//! | `net_udp_read`                 | [`RecvSource::send`]              |
//! | `read_block`                   | [`BlockStream::recv_block`]       |
//! | `write_block`                  | [`BlockStream::send_block`]       |
//! | Commands in Protocol Constants | [`Command`]                       |
//! | `DFT_PORT`                     | [`DEFAULT_TCP_PORT`]              |
//! | `NO_RESPONCE_INTERVAL`         | [`NO_RESPONSE_INTERVAL`]          |
//! | `PING_INTERVAL`                | [`PING_INTERVAL`]                 |

/// A collective [`Error`](std::error::Error) type that encompasses all the possible error cases
/// when sending, receiving or parsing data during networking operations.
#[derive(thiserror::Error, Debug)]
pub enum NetError {
    /// An Error ocurred within std giving a [`io::Error`]
    #[error(transparent)]
    IoError(#[from] std::io::Error),

    /// The received bytes do not follow a [`Block`]'s byte format.
    ///
    /// See [`BlockStream`] for more information.
    #[error("invalid bytes while parsing {location}")]
    InvalidBytes { location: &'static str },
}
