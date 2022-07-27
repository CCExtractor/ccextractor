use crate::{bindings::*, utils::is_true};
use log::debug;

const XDSClasses : [&'static str; 8] = ["Current", "Future", "Channel", "Miscellaneous", "Public Service", "Reserved", "Private data", "End"];
const NUM_XDS_BUFFERS: u8 = 9; // CEA recommends no more than one level of interleaving. Play it safe
const NUM_BYTES_PER_PACKET: u8 = 35; // Class + type (repeated for convenience) + data + zero


const XDS_CLASS_CURRENT : u8 = 0;
const XDS_CLASS_FUTURE : u8 = 1;
const XDS_CLASS_CHANNEL : u8 = 2;
const XDS_CLASS_MISC : u8 = 3;
const XDS_CLASS_PUBLIC : u8 = 4;
const XDS_CLASS_RESERVED : u8 = 5;
const XDS_CLASS_PRIVATE : u8 = 6;
const XDS_CLASS_END : u8 = 7;
const XDS_CLASS_OUT_OF_BAND: u8 = 0x49;

impl ccx_decoders_xds_context {
    pub fn do_end_of_xds(&mut self, sub: *mut cc_subtitle, expected_checksum: u8) {
        let mut cs = 0;

        unsafe {
            if self.cur_xds_buffer_idx == -1 || !is_true(self.xds_buffers[self.cur_xds_buffer_idx as usize].in_use as i32) { return };

            self.cur_xds_packet_class = self.xds_buffers[self.cur_xds_buffer_idx as usize].xds_class;
            self.cur_xds_payload = self.xds_buffers[self.cur_xds_buffer_idx as usize].bytes.as_mut_ptr();
            self.cur_xds_payload_length = self.xds_buffers[self.cur_xds_buffer_idx as usize].used_bytes as i32;
            let mut xds_data  = unsafe{std::slice::from_raw_parts_mut(self.cur_xds_payload,self.cur_xds_payload_length as usize)};

            self.cur_xds_packet_type = xds_data[1] as i32;

            xds_data[(self.cur_xds_payload_length + 1) as usize] = 0x0F;

            for i in 0..self.cur_xds_payload_length {
                cs = cs + xds_data[i as usize];
                cs = cs & 0x7f;
                let c = xds_data[i as usize] & 0x7F;
                debug!("{:02} - {} cs: {:02}\n", c, if c >= 0x20 {c} else {'?' as u8}, cs);
            }

            cs = (128 - cs) & 0x7F; // convert to 2's complement and discard high-order bit
            debug!("End of XDS, Class={} ({}), size={} Checksum OK: {} Used Buffers: {}\n", self.cur_xds_packet_class,
                XDSClasses[self.cur_xds_packet_class as usize], self.cur_xds_payload_length, (cs == expected_checksum) ,self.how_many_used() );

            if cs != expected_checksum || self.cur_xds_payload_length < 3 {
                debug!("Expected Checksum {:02} Calculated: {:02}", expected_checksum, cs);
                self.clear_xds_buffer(self.cur_xds_buffer_idx);
                return;
            }

            let mut was_proc = 0;
            if is_true(self.cur_xds_packet_type & 0x40) // Bit 6 set
            {
                self.cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND as i32;
            }

            match self.cur_xds_packet_class as u8
            {
                XDS_CLASS_FUTURE => {                            // Info on future program
                    // TODO
                    // if !(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS) // Don't bother processing something we don't need
                    // {
                    //     was_proc = 1;
                    // }
                }
                XDS_CLASS_CURRENT => { // Info on current program
                    was_proc = self.xds_do_current_and_future(sub);
                }
                XDS_CLASS_CHANNEL => {
                    was_proc = self.xds_do_channel(sub);
                }
                XDS_CLASS_MISC => {
                    was_proc = self.xds_do_misc();
                }
                XDS_CLASS_PRIVATE => { // CEA-608:
                    // The Private Data Class is for use in any closed system for whatever that
                    // system wishes. It shall not be defined by this standard now or in the future.
                    was_proc = self.xds_do_private_data(sub);
                }
                XDS_CLASS_OUT_OF_BAND => {
                    debug!("Out-of-band data, ignored.");
                    was_proc = 1;
                }
                _ => {}
            }

            if !is_true(was_proc)
            {
                debug!("Note: We found a currently unsupported XDS packet. \n");
                // TODO
                //dump(CCX_DMT_DECODER_XDS, self.cur_xds_payload, self.cur_xds_payload_length, 0, 0);
            }
            self.clear_xds_buffer( self.cur_xds_buffer_idx);
        }
    }

    pub fn how_many_used(&self) -> i32{
        let mut c = 0;
        for i in 0..NUM_XDS_BUFFERS {
            if is_true(self.xds_buffers[i as usize].in_use as i32) {
                c += 1;
            }
        }
        c
    }

    pub fn clear_xds_buffer(&mut self, num: i32) {
        self.xds_buffers[num as usize].in_use = 0;
        self.xds_buffers[num as usize].xds_class = -1;
        self.xds_buffers[num as usize].xds_type = -1;
        self.xds_buffers[num as usize].used_bytes = 0;

        self.xds_buffers[num as usize].bytes = [0; NUM_BYTES_PER_PACKET as usize];
    }

    pub fn xds_do_current_and_future(&mut self, sub: *mut cc_subtitle) -> i32 {
       unimplemented!();
    }
    pub fn xds_do_channel(&mut self, sub: *mut cc_subtitle) -> i32 {
        unimplemented!();
    }

    pub fn xds_do_misc(&mut self)  -> i32 {
        unimplemented!();
    }

    pub fn xds_do_private_data(&mut self, sub: *mut cc_subtitle) -> i32 {
        unimplemented!();
    }
}
