/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.h"
#include "include/version.h"

//!< @return malloc tag like string or 32bit hex dump.
const char *OffsetTable_SfntVersion_ToPrintString(uint32_t sfntVersion)
{
	char *sfntversionstr = malloc(strlen("0x12345678") + 1);
	ASSERT(sfntversionstr);

	bool isPrintable = true;
	for(int i = 0; i < 4; i++){
		char c = (uint8_t)(sfntVersion >> (8 * (3 - i)));
		if(0 == isprint(c)){
			isPrintable = false;
			break;
		}else{
			sfntversionstr[i] = c;
		}
	}
	sfntversionstr[4] = '\0';
	if(! isPrintable){
		sprintf(sfntversionstr, "0x%08x", sfntVersion);
	}

	return sfntversionstr;
}

int main(int argc, char **argv)
{
	/**
	第1引数でフォントファイル名を指定する
	*/
	if(argc < 2){
		return 1;
	}
	const char *fontfilepath = argv[1];

	int fd = open(fontfilepath, O_RDONLY, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}

	OffsetTable offsetTable;
	ssize_t ssize;
	ssize = read(fd, (void *)&offsetTable, sizeof(offsetTable));
	if(ssize != sizeof(offsetTable)){
		fprintf(stderr, "read: %zd %d %s\n", ssize, errno, strerror(errno));
		return 1;
	}
	offsetTable.sfntVersion		= ntohl(offsetTable.sfntVersion);
	offsetTable.numTables		= ntohs(offsetTable.numTables);

	const char *sfntversionstr = OffsetTable_SfntVersion_ToPrintString(offsetTable.sfntVersion);

	fprintf(stdout,
			"Font File Dumper: v %s\n"
			"project daisy bell 2019\n"
			"Dumping File:%s\n"
			"\n"
			"\n"
			"Offset Table\n"
			"------------\n"
			"	 sfnt version: 		%s\n"
			"	 number of tables:%3d\n",
			//FULL_VERSION,
			SHOW_VERSION,
			fontfilepath,
			sfntversionstr,
			offsetTable.numTables);

	close(fd);

	return 0;
}

