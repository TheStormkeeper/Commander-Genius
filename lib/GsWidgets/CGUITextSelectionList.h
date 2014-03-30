/*
 * CGUITextSelectionList.h
 *
 *  Created on: 01.11.2011
 *      Author: gerstrong
 *
 *  A Graphical List Control which has some item which are shown as Text and can be selected
 */

#ifndef CGUITEXTSELECTIONLIST_H_
#define CGUITEXTSELECTIONLIST_H_

#include <list>
#include <string>
#include "CGUIControl.h"
#include <widgets/GsScrollbar.h>

class CGUITextSelectionList : public CGUIControl
{
public:

    CGUITextSelectionList();

	void setConfirmButtonEvent(CEvent *ev);
	void setBackButtonEvent(CEvent *ev);

	bool sendEvent(const InputCommands command);
	void addText(const std::string &text);
	void processLogic();
	void processRender(const CRect<float> &RectDispCoordFloat);

    void drawScrollBar(const SDL_Rect &lRect);

	std::list<std::string> mItemList;

	int mHoverSelection;
	int mSelection;

private:

	std::shared_ptr<CEvent> mConfirmEvent;
	std::shared_ptr<CEvent> mBackEvent;
};

#endif /* CGUITEXTSELECTIONLIST_H_ */