// This file is @generated by prost-build.
#[derive(Clone, PartialEq, ::prost::Message)]
pub struct CcxSubEntryMessage {
    #[prost(int32, required, tag = "1")]
    pub eos: i32,
    #[prost(string, required, tag = "2")]
    pub stream_name: ::prost::alloc::string::String,
    #[prost(int64, required, tag = "3")]
    pub counter: i64,
    #[prost(int64, required, tag = "4")]
    pub start_time: i64,
    #[prost(int64, required, tag = "5")]
    pub end_time: i64,
    #[prost(string, repeated, tag = "7")]
    pub lines: ::prost::alloc::vec::Vec<::prost::alloc::string::String>,
}
