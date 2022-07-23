impl ccx_decoders_xds_context {
    pub fn do_end_of_xds(&mut self, sub: *mut cc_subtitle, expected_checksum: u8) {
        let mut cs = 0;
        // do it before calling.
        if ctx.is_null() { return; }

        unsafe {
            if self.cur_xds_buffer_idx == -1 || !self.xds_buffers[self.cur_xds_buffer_idx].in_use { return };

            self.cur_xds_packet_class = self.xds_buffers[self.cur_xds_buffer_idx].xds_class;
            self.cur_xds_payload = self.xds_buffers[self.cur_xds_buffer_idx].bytes;
            self.cur_xds_payload_length = self.xds_buffers[self.cur_xds_buffer_idx].used_bytes;
            self.cur_xds_packet_type = self.cur_xds_payload[1];
            self.cur_xds_payload[self.cur_xds_payload_length + 1] = 0x0F;

            for i in 0..self.cur_xds_payload_length {
                cs = cs + self.cur_xds_payload[i];
                cs = cs & 0x7f;
                let c = self.cur_xds_payload[i] & 0x7F;
                // TODO: common loggin debug_ftn
            }

            cs = (128 - cs) & 0x7F; // convert to 2's complement and discard high-order bit
            // TODO: common loggin debug_ftn

            if cs != expected_checksum || self.cur_xds_payload_length < 3 {
                // TODO : common loggin debug
                self.clear_xds_buffer(self.cur_xds_buffer_idx);
                return;
            }

            let mut was_proc = 0;

            if self.cur_xds_packet_type & 0x40 // Bit 6 set
            {
                self.cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND;
            }

            match self.cur_xds_packet_class
            {
                XDS_CLASS_FUTURE => {                            // Info on future program
                    if !(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS) // Don't bother processing something we don't need
                    {
                        was_proc = 1;
                    }
                }
                XDS_CLASS_CURRENT => { // Info on current program
                    was_proc = xds_do_current_and_future(sub, ctx);
                }
                XDS_CLASS_CHANNEL => {
                    was_proc = xds_do_channel(sub, ctx);
                }
                XDS_CLASS_MISC => {
                    was_proc = xds_do_misc(ctx);
                }
                XDS_CLASS_PRIVATE => { // CEA-608:
                    // The Private Data Class is for use in any closed system for whatever that
                    // system wishes. It shall not be defined by this standard now or in the future.
                    was_proc = xds_do_private_data(sub, ctx);
                }
                XDS_CLASS_OUT_OF_BAND => {
                    ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Out-of-band data, ignored.");
                    was_proc = 1;
                }
            }

            if !was_proc
            {
                ccx_common_logging.log_ftn("Note: We found a currently unsupported XDS packet.\n");
                dump(CCX_DMT_DECODER_XDS, self.cur_xds_payload, self.cur_xds_payload_length, 0, 0);
            }
            clear_xds_buffer(ctx, self.cur_xds_buffer_idx);
        }
    }

    pub fn clear_xds_buffer(&mut self, num: i32) {
        self.xds_buffers[num].in_use = 0;
        self.xds_buffers[num].xds_class = -1;
        self.xds_bufers[num].xds_type = -1;
        self.xds_buffers[num].used_types = 0;
        // TODO: what to do with MEMSET
    }

    pub fn xds_do_current_and_future(&mut self, cc_subtitle: *mut cc_subtitle) -> i32 {
       5
    }


}
