#ifndef UKMSG_H_
#define UKMSG_H_
#include "uKTag.h"
#include "cEnumeratedList.h"

typedef cEnumeratedList<uKTag> tagList;

/**Keystone Message definition. Valid messages has an ID and required tags which is defined for each tag,
 * these should be defined in the dictionary */
struct uKMsgDef
{
    const uint8_t mMsgID;		/**< Message ID */
    const uint8_t mReqCount;	/**< Amount of required tags for the message */
    const uint8_t* mReqTags;	/**< Pointer to required tag definitions */
};

/**Keystone Message dictionary. Dictionary message structure which holds the amount of messages in the dictionary,
 * these should be defined in the dictionary */
struct uKMsgDict
{
	const uint8_t mMsgCount;	/**< Amount of messages mMsgs points to */
	const uKMsgDef* mMsgs;		/**< Pointer to message definitions */
};


/**micro Keystone Message. Tags are added into a message */
class uKMsg
{

	uint8_t mMsgID;	/**< Message ID BYTE[0]. Located in the first byte of a message */
	tagList mLst;	/**< Tag list. List of tags in the message object */

	uKMsgDict* mMsgDict;	/**<Pointer to the defined uKMsg dictionary */

	bool findTag(uint8_t id);
	bool checkMessage();

public:

	uKMsg(const uKMsgDict* msgdict, const uKTagDict* tagdict, uint8_t* buff, uint32_t buffLen);
	uKMsg(const uKMsgDict* msgdict, uint8_t id);

	/**Message ID. @retval mMsgID Message ID */
	uint8_t id(){ return mMsgID; };

	/**Get first Tag in message list. @retval t Pointer to first tag in list. Returns zero when there is no tags in the list */
	uKTag* firstTag(){ return mLst.first(); };

	/**Get next Tag in message list. @retval t Pointer to next tag in list. Returns zero when there is no tags in the list */
	uKTag* nextTag(){ return mLst.getNext(); };

	void addTag( uKTag* t);
	uint32_t getBuff(void* buff, uint32_t buffLen);


	void print();
	~uKMsg();
};

#endif /* UKMSG_H_ */
