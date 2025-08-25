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
