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

mod common;
mod source;
mod target;

pub use common::*;
pub use source::*;
pub use target::*;
