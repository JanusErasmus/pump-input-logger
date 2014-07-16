#ifndef UKTAG_H_
#define UKTAG_H_
#include <stdint.h>

//Maximum value for 8 bit uint8_t
#define UKTAG_DATA_LIMIT 	255
#define UKTAG_DATA_LEN		64

/**Keystone Tag definition. Valid tags has a data length and ID which is defined for each tag,
 * these should be defined in the dictionary */
struct uKTagDef
{
    const uint8_t mTagID; 		/**< Tag ID  */
    const uint8_t mTagDataLen;	/**< Amount of data bytes in the tag */
};

/**Keystone Tag dictionary. Dictionary tag structure which holds the amount of tags in the dictionary,
 * these should be defined in the dictionary */
struct uKTagDict
{
	const uint8_t mTagsCount;	/**< Amount of tags mTags points to */
	const uKTagDef* mTags;		/**< Pointer to the tag definitions */
};

/**micro Keystone Tag. Tags that are appended into a micro Keystone message (uKMsg) */
class uKTag
{
	uint8_t mTagID;		/**< Tag ID BYTE[0]. Located in the first byte of a tag */
	uint8_t mDataLen;	/**< Tag length BYTE[1]. Located in the second byte of a tag */
	uint8_t* mData;		/**< Tag length BYTE[2..n]. BYTE[2] through to BYTE[2 + mDataLen] is the tag data */

	void setData(void* buff);

public:
	uKTag(const uKTagDict* tagdict, uint8_t* buff, uint8_t buffLen);
	uKTag(const uKTagDict* tagdict, uint8_t tagID, void* data);
	~uKTag();

	/**Tag ID. @retval mTagID Tag ID */
	uint8_t id(){ return mTagID; };
	uint8_t dataLenght();
	uint8_t data(void* buff, uint8_t buffLen);

	uint8_t getBuff(void* buff, uint8_t buffLen);

	void print();
};

#endif /* UKTAG_H_ */
