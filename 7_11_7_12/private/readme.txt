The challenge is about modification of the next_header_offset field in the 7zip header, which allows us to make a gap in the file structure to insert another file.

Part1 is just another 7zip file inserted into that gap, it should be easy enough to extract it (just remove all the content up to the second 7zip header)

Part2 is about extracting it automatically, which is a bit trickier. The way how it is meant to be solved is to parse 7zip headers and count size of all the streams in the file, which should be the original next_header_offset. Another way I can think of is probably just to repack the extracted files from the archive, and use the next_header_offset value from it, it might not work in all cases but I think it might do it in this scenario.
