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
//! | `start_tcp_srv`                | [`RecvSource::new`]               |
//! | `start_udp_srv`                | [`RecvSource::new`]               |
//! | `net_tcp_read`                 | [`RecvSource::recv_header_or_cc`] |
//! | `net_udp_read`                 | [`RecvSource::send`]              |
//! | `read_block`                   | [`BlockStream::recv_block`]       |
//! | `write_block`                  | [`BlockStream::send_block`]       |
//! | `net_check_conn`               | [`SendTarget::check_connection`]  |
//! | Commands in Protocol Constants | [`Command`]                       |
//! | `DFT_PORT`                     | [`DEFAULT_TCP_PORT`]              |
//! | `NO_RESPONCE_INTERVAL`         | [`NO_RESPONSE_INTERVAL`]          |
//! | `PING_INTERVAL`                | [`PING_INTERVAL`]                 |

mod common;
mod source;
mod target;

pub mod c_functions;

pub use common::*;
pub use source::*;
pub use target::*;
