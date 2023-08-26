use crate::util::net::*;
use std::sync::RwLock;

static TARGET: RwLock<Option<SendTarget>> = RwLock::new(None);
static SOURCE: RwLock<Option<RecvSource>> = RwLock::new(None);

/// Rust equivalent for `connect_to_srv` function in C. Uses Rust-native types as input and output.
pub fn connect_to_srv(
    addr: &'static str,
    port: Option<u16>,
    cc_desc: Option<&'static str>,
    pwd: Option<&'static str>,
) {
    let mut send_target = TARGET.write().unwrap();
    *send_target = Some(SendTarget::new(SendTargetConfig {
        target_addr: addr,
        port,
        password: pwd,
        description: cc_desc,
    }));
}

/// Rust equivalent for `net_send_header` function in C. Uses Rust-native types as input and output.
pub fn net_send_header(data: &[u8]) {
    let mut send_target = TARGET.write().unwrap();
    send_target.as_mut().unwrap().send_header(data);
}

/// Rust equivalent for `net_send_cc` function in C. Uses Rust-native types as input and output.
pub fn net_send_cc(data: &[u8]) -> bool {
    let mut send_target = TARGET.write().unwrap();
    send_target.as_mut().unwrap().send_cc(data)
}

/// Rust equivalent for `net_check_conn` function in C. Uses Rust-native types as input and output.
pub fn net_check_conn() {
    let mut send_target = TARGET.write().unwrap();
    send_target.as_mut().unwrap().check_connection();
}

/// Rust equivalent for `net_send_epg` function in C. Uses Rust-native types as input and output.
pub fn net_send_epg(
    start: &str,
    stop: &str,
    title: Option<&str>,
    desc: Option<&str>,
    lang: Option<&str>,
    category: Option<&str>,
) {
    let mut send_target = TARGET.write().unwrap();
    send_target
        .as_mut()
        .unwrap()
        .send_epg_data(start, stop, title, desc, lang, category);
}

/// Rust equivalent for `net_tcp_read` function in C. Uses Rust-native types as input and output.
pub fn net_tcp_read(buffer: &mut [u8]) -> Option<usize> {
    let mut recv_source = SOURCE.write().unwrap();
    if let Ok(b) = recv_source.as_mut().unwrap().recv_header_or_cc() {
        if let Some(block) = b {
            buffer[..block.data().len()].copy_from_slice(block.data());
            Some(block.data().len())
        } else {
            Some(0)
        }
    } else {
        None
    }
}

/// Rust equivalent for `net_udp_read` function in C. Uses Rust-native types as input and output.
pub fn net_udp_read(buffer: &mut [u8]) -> Option<usize> {
    let mut recv_source = SOURCE.write().unwrap();
    recv_source.as_mut().unwrap().recv(buffer).ok()
}

/// Rust equivalent for `start_tcp_srv` function in C. Uses Rust-native types as input and output.
pub fn start_tcp_srv(port: Option<u16>, pwd: Option<&'static str>) {
    let mut recv_source = SOURCE.write().unwrap();
    *recv_source = Some(RecvSource::new(RecvSourceConfig::Tcp {
        port,
        password: pwd,
    }));
}

/// Rust equivalent for `start_udp_srv` function in C. Uses Rust-native types as input and output.
pub fn start_udp_srv(src: Option<&'static str>, addr: Option<&'static str>, port: u16) {
    let mut recv_source = SOURCE.write().unwrap();
    *recv_source = Some(RecvSource::new(RecvSourceConfig::Udp {
        source: src,
        address: addr,
        port,
    }));
}
