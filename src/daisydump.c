/**
  @file
  @author michianri.nukazawa@gmail.com / project daisy bell
  @details license: MIT
 */

#include "src/OpenType.h"
#include "include/version.h"

//!< @return malloc tag like string or 32bit hex dump.
const char *TagType_ToPrintString(uint32_t tagValue)
{
	char *tagstr = malloc(strlen("0x12345678") + 1);
	ASSERT(tagstr);

	bool isPrintable = true;
	for(int i = 0; i < 4; i++){
		char c = (uint8_t)(tagValue >> (8 * (3 - i)));
		if(0 == isprint(c)){
			isPrintable = false;
			break;
		}else{
			tagstr[i] = c;
		}
	}
	tagstr[4] = '\0';
	if(! isPrintable){
		sprintf(tagstr, "0x%08x", tagValue);
	}

	return tagstr;
}

const char *OffsetTable_SfntVersion_ToPrintString(uint32_t sfntVersion)
{
	return TagType_ToPrintString(sfntVersion);
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

	// ** OffsetTable
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

	// ** TableDirectory
	TableDirectory_Member *tableDirectory = NULL;
	for(int i = 0; i < offsetTable.numTables; i++){
		TableDirectory_Member tableDirectory_Member;
		ssize_t ssize;
		ssize = read(fd, (void *)&tableDirectory_Member, sizeof(tableDirectory_Member));
		if(ssize != sizeof(tableDirectory_Member)){
			fprintf(stderr, "read: %zd %d %s\n", ssize, errno, strerror(errno));
			return 1;
		}
		tableDirectory_Member.tag	= ntohl(tableDirectory_Member.tag);
		tableDirectory_Member.checkSum	= ntohl(tableDirectory_Member.checkSum);
		tableDirectory_Member.offset	= ntohl(tableDirectory_Member.offset);
		tableDirectory_Member.length	= ntohl(tableDirectory_Member.length);

		tableDirectory = realloc(tableDirectory, sizeof(TableDirectory_Member) * (i + 1));
		ASSERT(tableDirectory);
		memcpy(&tableDirectory[i], &tableDirectory_Member, sizeof(TableDirectory_Member));

		const char *tagstring = TagType_ToPrintString(tableDirectory_Member.tag);
		fprintf(stdout,
				"%2d. '%s' - checksum = 0x%08x, offset = 0x%08x(%6d), len =%8d\n",
				i,
				tagstring,
				tableDirectory_Member.checkSum,
				tableDirectory_Member.offset,
				tableDirectory_Member.offset,
				tableDirectory_Member.length);
	}

	close(fd);

	return 0;
}

