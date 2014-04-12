/********************************************************
256 BYTES IS ENOUGH FOR ALL THE SUPPORTED CHARACTERS IN
EIA-708, SO INTERNALLY WE USE THIS TABLE (FOR CONVENIENCE)

00-1F -> Characters that are in the G2 group in 20-3F,
         except for 06, which is used for the closed captions
         sign "CC" which is defined in group G3 as 00. (this 
         is by the article 33).
20-7F -> Group G0 as is - corresponds to the ASCII code 
80-9F -> Characters that are in the G2 group in 60-7F 
         (there are several blank characters here, that's OK)
A0-FF -> Group G1 as is - non-English characters and symbols
*/

unsigned char get_internal_from_G0 (unsigned char g0_char)
{
    return g0_char;
}

unsigned char get_internal_from_G1 (unsigned char g1_char)
{
    return g1_char;
}

// TODO: Probably not right
// G2: Extended Control Code Set 1
unsigned char get_internal_from_G2 (unsigned char g2_char)
{
    if (g2_char>=0x20 && g2_char<=0x3F)
        return g2_char-0x20;
    if (g2_char>=0x60 && g2_char<=0x7F)
        return g2_char+0x20;
    // Rest unmapped, so we return a blank space
    return 0x20; 
}

// TODO: Probably not right
// G3: Future Characters and Icon Expansion
unsigned char get_internal_from_G3 (unsigned char g3_char)
{
    if (g3_char==0xa0) // The "CC" (closed captions) sign
        return 0x06; 
    // Rest unmapped, so we return a blank space
    return 0x20; 
}
