#include "ccextractor.h"


// Parse the user data for captions. The udtype variable denotes
// to which type of data it belongs:
// 0 .. sequence header
// 1 .. GOP header
// 2 .. picture header
// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if ustream->error
// is FALSE, parsing can set ustream->error to TRUE.
int user_data(struct bitstream *ustream, int udtype)
{
    dbg_print(CCX_DMT_VERBOSE, "user_data(%d)\n", udtype);

    // Shall not happen
    if (ustream->error || ustream->bitsleft <= 0)
	{
		// ustream->error=1;
		return 0; // Actually discarded on call.
		// CFS: Seen in a Wobble edited file.
        // fatal(EXIT_BUG_BUG, "user_data: Impossible!");
	}

    // Do something
    stat_numuserheaders++;
    //header+=4;

    unsigned char *ud_header = next_bytes(ustream, 4);
    if (ustream->error || ustream->bitsleft <= 0)
	{		
		return 0;  // Actually discarded on call.
		// CFS: Seen in Stick_VHS.mpg. 
        // fatal(EXIT_BUG_BUG, "user_data: Impossible!");
	}

    // DVD CC header, see
    // <http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/SCC_FORMAT.HTML>
    if ( !memcmp(ud_header,"\x43\x43", 2 ) )
    {
        stat_dvdccheaders++;

        // Probably unneeded, but keep looking for extra caption blocks
        int maybeextracb = 1;

        read_bytes(ustream, 4); // "43 43 01 F8"

        unsigned char pattern_flag = (unsigned char) read_bits(ustream,1);
        read_bits(ustream,1);
        int capcount=(int) read_bits(ustream,5);
        int truncate_flag = (int) read_bits(ustream,1); // truncate_flag - one CB extra

        int field1packet = 0; // expect Field 1 first
        if (pattern_flag == 0x00) 
            field1packet=1; // expect Field 1 second

        dbg_print(CCX_DMT_VERBOSE, "Reading %d%s DVD CC segments\n",
               capcount, (truncate_flag?"+1":""));

        capcount += truncate_flag;

        // This data comes before the first frame header, so
        // in order to get the correct timing we need to set the
        // current time to one frame after the maximum time of the
        // last GOP.  Only usefull when there are frames before
        // the GOP.
        if (fts_max > 0)
            fts_now = fts_max + LLONG(1000.0/current_fps);

        int rcbcount = 0;
        for (int i=0; i<capcount; i++)
        {
            for (int j=0;j<2;j++)
            {
                unsigned char data[3];
                data[0]=read_u8(ustream);
                data[1]=read_u8(ustream);
                data[2]=read_u8(ustream);

                // Obey the truncate flag.
                if ( truncate_flag && i == capcount-1 && j == 1 )
                {
                    maybeextracb = 0;
                    break;
                }
                /* Field 1 and 2 data can be in either order,
                   with marker bytes of \xff and \xfe
                   Since markers can be repeated, use pattern as well */
                if ((data[0]&0xFE) == 0xFE) // Check if valid
                { 
                    if (data[0]==0xff && j==field1packet)
                        data[0]=0x04; // Field 1
                    else
                        data[0]=0x05; // Field 2
                    do_cb(data);
                    rcbcount++;
                }
                else
                {
                    dbg_print(CCX_DMT_VERBOSE, "Illegal caption segment - stop here.\n");
                    maybeextracb = 0;
                    break;
                }
            }
        }
        // Theoretically this should not happen, oh well ...
        // Deal with extra closed captions some DVD have.
        int ecbcount = 0;
        while ( maybeextracb && (next_u8(ustream)&0xFE) == 0xFE )
        {
            for (int j=0;j<2;j++)
            {
                unsigned char data[3];
                data[0]=read_u8(ustream);
                data[1]=read_u8(ustream);
                data[2]=read_u8(ustream);
                /* Field 1 and 2 data can be in either order,
                   with marker bytes of \xff and \xfe
                   Since markers can be repeated, use pattern as well */
                if ((data[0]&0xFE) == 0xFE) // Check if valid
                { 
                    if (data[0]==0xff && j==field1packet)
                        data[0]=0x04; // Field 1
                    else
                        data[0]=0x05; // Field 2
                    do_cb(data);
                    ecbcount++;
                }
                else
                {
                    dbg_print(CCX_DMT_VERBOSE, "Illegal (extra) caption segment - stop here.\n");
                    maybeextracb = 0;
                    break;
                }
            }
        }

        dbg_print(CCX_DMT_VERBOSE, "Read %d/%d DVD CC blocks\n", rcbcount, ecbcount);
    }
    // SCTE 20 user data
    else if (ud_header[0] == 0x03)
    {
        if ((ud_header[1]&0x7F) == 0x01)
        {
            unsigned char cc_data[3*31+1]; // Maximum cc_count is 31

            stat_scte20ccheaders++;
            read_bytes(ustream, 2); // "03 01"

            unsigned cc_count = (unsigned int) read_bits(ustream,5);
            dbg_print(CCX_DMT_VERBOSE, "Reading %d SCTE 20 CC blocks\n", cc_count);

            unsigned field_number;
            unsigned cc_data1;
            unsigned cc_data2;
            unsigned marker;
            for (unsigned j=0;j<cc_count;j++)
            {
                skip_bits(ustream,2); // priority - unused
                field_number = (unsigned int) read_bits(ustream,2);
                skip_bits(ustream,5); // line_offset - unused
                cc_data1 = (unsigned int) read_bits(ustream,8);
                cc_data2 = (unsigned int) read_bits(ustream,8);
                marker = (unsigned int)read_bits(ustream,1); // TODO: Add syntax check

                if (ustream->bitsleft < 0)
                    fatal(EXIT_BUG_BUG, "Oops!");

                // Field_number is either
                //  0 .. forbiden
                //  1 .. field 1 (odd)
                //  2 .. field 2 (even)
                //  3 .. repeated, from repeat_first_field, effectively field 1
                if (field_number < 1)
                {
                    // 0 is invalid
                    cc_data[j*3]=0x00; // Set to invalid
                    cc_data[j*3+1]=0x00;
                    cc_data[j*3+2]=0x00;
                }
                else
                {
                    // Treat field_number 3 as 1
                    field_number = (field_number - 1) & 0x01;
                    // top_field_first also affects to which field the caption
                    // belongs.
                    if(!top_field_first)
                        field_number ^= 0x01;
                    cc_data[j*3]=0x04|(field_number);
                    cc_data[j*3+1]=reverse8(cc_data1);
                    cc_data[j*3+2]=reverse8(cc_data2);
                }
            }
            cc_data[cc_count*3]=0xFF;
            store_hdcc(cc_data, cc_count, current_tref, fts_now);

            dbg_print(CCX_DMT_VERBOSE, "Reading SCTE 20 CC blocks - done\n");
        }
        // reserved - unspecified
    }
    // ReplayTV 4000/5000 caption header - parsing information
    // derived from CCExtract.bdl
    else if ( (ud_header[0] == 0xbb     //ReplayTV 4000 
               || ud_header[0] == 0x99) //ReplayTV 5000 
              && ud_header[1] == 0x02 )
    {
        unsigned char data[3];
        if (ud_header[0]==0xbb)
            stat_replay4000headers++;
        else
            stat_replay5000headers++;

        read_bytes(ustream, 2); // "BB 02" or "99 02"
        data[0]=0x05; // Field 2
        data[1]=read_u8(ustream);
        data[2]=read_u8(ustream);
        do_cb(data);
        read_bytes(ustream, 2); // Skip "CC 02" for R4000 or "AA 02" for R5000
        data[0]=0x04; // Field 1
        data[1]=read_u8(ustream);
        data[2]=read_u8(ustream);
        do_cb(data);
    }
    // HDTV - see A/53 Part 4 (Video)
    else if ( !memcmp(ud_header,"\x47\x41\x39\x34", 4 ) )
    {
        stat_hdtv++;

        read_bytes(ustream, 4); // "47 41 39 34"

        unsigned char type_code = read_u8(ustream);
        if (type_code==0x03) // CC data.
        {
            skip_bits(ustream,1); // reserved
            unsigned char process_cc_data = (unsigned char) read_bits(ustream,1);
            skip_bits(ustream,1); // additional_data - unused
            unsigned char cc_count = (unsigned char) read_bits(ustream,5);
            read_bytes(ustream, 1); // "FF"
            if (process_cc_data)
            {
                dbg_print(CCX_DMT_VERBOSE, "Reading %d HDTV CC blocks\n", cc_count);

                int proceed = 1;
                unsigned char *cc_data = read_bytes(ustream, cc_count*3);
                if (ustream->bitsleft < 0)
                    fatal(EXIT_BUG_BUG, "Not enough for CC captions!");

                // Check for proper marker - This read makes sure that
                // cc_count*3+1 bytes are read and available in cc_data.
                if (read_u8(ustream)!=0xFF)
                    proceed=0;

                if (!proceed)
                {
                    dbg_print(CCX_DMT_VERBOSE, "\rThe following payload is not properly terminated.\n");
                    dump (CCX_DMT_VERBOSE, cc_data, cc_count*3+1, 0, 0);
                }
                dbg_print(CCX_DMT_VERBOSE, "Reading %d HD CC blocks\n", cc_count);

                // B-frames might be (temporal) before or after the anchor
                // frame they belong to. Store the buffer until the next anchor
                // frame occurs.  The buffer will be flushed (sorted) in the
                // picture header (or GOP) section when the next anchor occurs.
                // Please note we store the current value of the global
                // fts_now variable (and not get_fts()) as we are going to
                // re-create the timeline in process_hdcc() (Slightly ugly).
                store_hdcc(cc_data, cc_count, current_tref, fts_now);

                dbg_print(CCX_DMT_VERBOSE, "Reading HDTV blocks - done\n");
            }
        }
        // reserved - additional_cc_data
    }
    // DVB closed caption header for Dish Network (Field 1 only) */
    else if ( !memcmp(ud_header,"\x05\x02", 2 ) )
    {
        // Like HDTV (above) Dish Network captions can be stored at each
        // frame, but maximal two caption blocks per frame and only one
        // field is stored.
        // To process this with the HDTV framework we create a "HDTV" caption
        // format compatible array. Two times 3 bytes plus one for the 0xFF
        // marker at the end. Pre-init to field 1 and set the 0xFF marker.
        static unsigned char dishdata[7] = {0x04, 0, 0, 0x04, 0, 0, 0xFF};
        int cc_count;

        dbg_print(CCX_DMT_VERBOSE, "Reading Dish Network user data\n");

        stat_dishheaders++;

        read_bytes(ustream, 2); // "05 02"

        // The next bytes are like this:
        // header[2] : ID: 0x04 (MPEG?), 0x03 (H264?)
        // header[3-4]: Two byte counter (counting (sub-)GOPs?)
        // header[5-6]: Two bytes, maybe checksum?
        // header[7]: Pattern type
        //            on B-frame: 0x02, 0x04
        //            on I-/P-frame: 0x05
        unsigned char id = read_u8(ustream);
        unsigned dishcount = read_u16(ustream);
        unsigned something = read_u16(ustream);
        unsigned char type = read_u8(ustream);
        dbg_print(CCX_DMT_PARSE, "DN  ID: %02X  Count: %5u  Unknown: %04X  Pattern: %X",
             id, dishcount, something, type);

        unsigned char hi;

        // The following block needs 4 to 6 bytes starting from the
        // current position
        unsigned char *dcd = ustream->pos; // dish caption data
        switch (type)
        {
        case 0x02:
            // Two byte caption - always on B-frame
            // The following 4 bytes are:
            // 0  :  0x09
            // 1-2: caption block
            // 3  : REPEAT - 02: two bytes
            //             - 04: four bytes (repeat first two)
            dbg_print(CCX_DMT_PARSE, "\n02 %02X  %02X:%02X - R:%02X :",
                       dcd[0], dcd[1], dcd[2], dcd[3]);

            cc_count = 1;
            dishdata[1]=dcd[1];
            dishdata[2]=dcd[2];

            dbg_print(CCX_DMT_PARSE, "%s", debug_608toASC( dishdata, 0) );

            type=dcd[3];  // repeater (0x02 or 0x04)
            hi = dishdata[1] & 0x7f; // Get only the 7 low bits
            if (type==0x04 && hi<32) // repeat (only for non-character pairs)
            {
                cc_count = 2;
                dishdata[3]=0x04; // Field 1
                dishdata[4]=dishdata[1];
                dishdata[5]=dishdata[2];

                dbg_print(CCX_DMT_PARSE, "%s:\n", debug_608toASC( dishdata+3, 0) );
            }
            else
            {
                dbg_print(CCX_DMT_PARSE, ":\n");
            }

            dishdata[cc_count*3] = 0xFF; // Set end marker

            store_hdcc(dishdata, cc_count, current_tref, fts_now);

            // Ignore 3 (0x0A, followed by two unknown) bytes.
            break;
        case 0x04:
            // Four byte caption - always on B-frame
            // The following 5 bytes are:
            // 0  :  0x09
            // 1-2: caption block
            // 3-4: caption block
            dbg_print(CCX_DMT_PARSE, "\n04 %02X  %02X:%02X:%02X:%02X  :",
                       dcd[0], dcd[1], dcd[2], dcd[3], dcd[4]);

            cc_count = 2;
            dishdata[1]=dcd[1];
            dishdata[2]=dcd[2];
	    
            dishdata[3]=0x04; // Field 1
            dishdata[4]=dcd[3];
            dishdata[5]=dcd[4];
            dishdata[6] = 0xFF; // Set end marker

            dbg_print(CCX_DMT_PARSE, "%s", debug_608toASC( dishdata, 0) );
            dbg_print(CCX_DMT_PARSE, "%s:\n", debug_608toASC( dishdata+3, 0) );
          
            store_hdcc(dishdata, cc_count, current_tref, fts_now);

            // Ignore 4 (0x020A, followed by two unknown) bytes.
            break;
        case 0x05:
            // Buffered caption - always on I-/P-frame
            // The following six bytes are:
            // 0  :  0x04
            // - the following are from previous 0x05 caption header -
            // 1  : prev dcd[2]
            // 2-3: prev dcd[3-4] 
            // 4-5: prev dcd[5-6]
            dbg_print(CCX_DMT_PARSE, " - %02X  pch: %02X %5u %02X:%02X\n",
                       dcd[0], dcd[1],
                       (unsigned)dcd[2]*256+dcd[3],
                       dcd[4], dcd[5]);
            dcd+=6; // Skip these 6 bytes

            // Now one of the "regular" 0x02 or 0x04 captions follows
            dbg_print(CCX_DMT_PARSE, "%02X %02X  %02X:%02X",
                       dcd[0], dcd[1], dcd[2], dcd[3]);

            type=dcd[0]; // Number of caption bytes (0x02 or 0x04)

            cc_count = 1;
            dishdata[1]=dcd[2];
            dishdata[2]=dcd[3];

            dcd+=4; // Skip the first 4 bytes.
            if (type==0x02)
            {
                type=dcd[0]; // repeater (0x02 or 0x04)
                dcd++; // Skip the repeater byte.

                dbg_print(CCX_DMT_PARSE, " - R:%02X :%s", type, debug_608toASC( dishdata, 0) );

                hi = dishdata[1] & 0x7f; // Get only the 7 low bits
                if (type==0x04 && hi<32)
                {
                    cc_count = 2;
                    dishdata[3]=0x04; // Field 1
                    dishdata[4]=dishdata[1];
                    dishdata[5]=dishdata[2];
                    dbg_print(CCX_DMT_PARSE, "%s:\n", debug_608toASC( dishdata+3, 0) );
                }
                else
                {
                    dbg_print(CCX_DMT_PARSE, ":\n");
                }
                dishdata[cc_count*3] = 0xFF; // Set end marker
            }
            else
            {
                dbg_print(CCX_DMT_PARSE, ":%02X:%02X  ",
                           dcd[0], dcd[1]);
                cc_count = 2;
                dishdata[3]=0x04; // Field 1
                dishdata[4]=dcd[0];
                dishdata[5]=dcd[1];
                dishdata[6] = 0xFF; // Set end marker
		
                dbg_print(CCX_DMT_PARSE, ":%s", debug_608toASC( dishdata, 0) );
                dbg_print(CCX_DMT_PARSE, "%s:\n", debug_608toASC( dishdata+3, 0) );                
            }
	    
            store_hdcc(dishdata, cc_count, current_tref, fts_now);
	    
            // Ignore 3 (0x0A, followed by 2 unknown) bytes.
            break;
        default:
            // printf ("Unknown?\n");
            break;
        } // switch

        dbg_print(CCX_DMT_VERBOSE, "Reading Dish Network user data - done\n");
    }
    // CEA 608 / aka "Divicom standard", see:
    // http://www.pixeltools.com/tech_tip_closed_captioning.html
    else if ( !memcmp(ud_header,"\x02\x09", 2 ) )
    {
        // Either a documentation or more examples are needed.
        stat_divicom++;

        unsigned char data[3];

        read_bytes(ustream, 2); // "02 09"
        read_bytes(ustream, 2); // "80 80" ???
        read_bytes(ustream, 2); // "02 0A" ???
        data[0]=0x04; // Field 1
        data[1]=read_u8(ustream);
        data[2]=read_u8(ustream);
        do_cb(data);
        // This is probably incomplete!
    }
    else
    {
        // Some other user data
        // 06 02 ... Seems to be DirectTV
        dbg_print(CCX_DMT_VERBOSE, "Unrecognized user data:\n");
        int udatalen = ustream->end - ustream->pos;
        dump (CCX_DMT_VERBOSE, ustream->pos, (udatalen > 128 ? 128 : udatalen),0 ,0);
    }

    dbg_print(CCX_DMT_VERBOSE, "User data - processed\n");

    // Read complete
    return 1;
}
