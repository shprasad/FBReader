/*
 * FBReader -- electronic book reader
 * Copyright (C) 2004-2007 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <algorithm>

#include <ZLUnicodeUtil.h>

#include <ZLTextHyphenator.h>

#include "ZLTextView.h"
#include "ZLTextLineInfo.h"
#include "ZLTextParagraphCursor.h"
#include "ZLTextStyle.h"
#include "ZLTextWord.h"

ZLTextLineInfoPtr TextView::processTextLine(const WordCursor &start, const WordCursor &end) {
	ZLTextLineInfoPtr infoPtr = new ZLTextLineInfo(start, myStyle.style());
	ZLTextLineInfo &info = *infoPtr;

	std::set<ZLTextLineInfoPtr>::const_iterator it = myLineInfoCache.find(infoPtr);
	if (it != myLineInfoCache.end()) {
		const ZLTextLineInfoPtr &storedInfo = *it;
		myStyle.applyControls(storedInfo->Start, storedInfo->End);
		return storedInfo;
	}

	WordCursor current = start;
	const ParagraphCursor &paragraphCursor = current.paragraphCursor();
	const bool isFirstLine = current.isStartOfParagraph();

	if (paragraphCursor.paragraph().kind() == ZLTextParagraph::TREE_PARAGRAPH) {
		info.NodeInfo = new TreeNodeInfo();
		TreeNodeInfo &nodeInfo = *info.NodeInfo;
		const ZLTextTreeParagraph &treeParagraph = (const ZLTextTreeParagraph&)paragraphCursor.paragraph();
		nodeInfo.IsLeaf = treeParagraph.children().empty();
		nodeInfo.IsOpen = treeParagraph.isOpen();
		nodeInfo.IsFirstLine = isFirstLine;
		nodeInfo.ParagraphNumber = paragraphCursor.index();

		nodeInfo.VerticalLinesStack.reserve(treeParagraph.depth() - 1);
		if (treeParagraph.depth() > 1) {
			const ZLTextTreeParagraph *current = treeParagraph.parent();
			nodeInfo.VerticalLinesStack.push_back(current->children().back() != &treeParagraph);
			for (int i = 1; i < treeParagraph.depth() - 1; ++i) {
				const ZLTextTreeParagraph *parent = current->parent();
				nodeInfo.VerticalLinesStack.push_back(current != parent->children().back());
				current = parent;
			}
		}
	}

	if (isFirstLine) {
		TextElement::Kind elementKind = paragraphCursor[current.wordNumber()].kind();
		while ((elementKind == TextElement::CONTROL_ELEMENT) ||
					 (elementKind == TextElement::FORCED_CONTROL_ELEMENT)) {
			const TextElement &element = paragraphCursor[current.wordNumber()];
			switch (elementKind) {
				case TextElement::CONTROL_ELEMENT:
					myStyle.applyControl((const ControlElement&)element);
					break;
				case TextElement::FORCED_CONTROL_ELEMENT:
					myStyle.applyControl((const ForcedControlElement&)element);
					break;
				default:
					break;
			}
			current.nextWord();
			if (current.equalWordNumber(end)) {
				break;
			}
			elementKind = paragraphCursor[current.wordNumber()].kind();
		}
		info.StartStyle = myStyle.style();
		info.RealStart = current;
	}

	ZLTextStylePtr storedStyle = myStyle.style();

	info.LeftIndent = myStyle.style()->leftIndent();
	if (isFirstLine) {
		info.LeftIndent += myStyle.style()->firstLineIndentDelta();
	}
	if (!info.NodeInfo.isNull()) {
		info.LeftIndent += (myStyle.context().stringHeight() + 2) / 3 * 4 * (info.NodeInfo->VerticalLinesStack.size() + 1);
	}
	info.Width = info.LeftIndent;

	if (info.RealStart.equalWordNumber(end)) {
	  info.End = info.RealStart;
		return infoPtr;
	}

	int newWidth = info.Width;
	int newHeight = info.Height;
	int newDescent = info.Descent;
	int maxWidth = myStyle.context().width() - myStyle.style()->rightIndent();
	bool wordOccured = false;
	bool isVisible = false;
	int lastSpaceWidth = 0;
	int internalSpaceCounter = 0;
	int removeLastSpace = false;

	TextElement::Kind elementKind = paragraphCursor[current.wordNumber()].kind();

	do {
		const TextElement &element = paragraphCursor[current.wordNumber()];
		newWidth += myStyle.elementWidth(element, current.charNumber());
		newHeight = std::max(newHeight, myStyle.elementHeight(element));
		newDescent = std::max(newDescent, myStyle.elementDescent(element));
		switch (elementKind) {
			case TextElement::CONTROL_ELEMENT:
				myStyle.applyControl((const ControlElement&)element);
				break;
			case TextElement::FORCED_CONTROL_ELEMENT:
				myStyle.applyControl((const ForcedControlElement&)element);
				break;
			case TextElement::WORD_ELEMENT:
			case TextElement::IMAGE_ELEMENT:
				wordOccured = true;
				isVisible = true;
				break;
			case TextElement::HSPACE_ELEMENT:
				if (wordOccured) {
					wordOccured = false;
					++internalSpaceCounter;
					lastSpaceWidth = myStyle.context().spaceWidth();
					newWidth += lastSpaceWidth;
				}
				break;
			case TextElement::EMPTY_LINE_ELEMENT:
				isVisible = true;
			default:
				break;
		}

		if ((newWidth > maxWidth) && !info.End.equalWordNumber(start)) {
			break;
		}

		TextElement::Kind previousKind = elementKind;
		current.nextWord();
		bool allowBreak = current.equalWordNumber(end);
		if (!allowBreak) {
			elementKind = paragraphCursor[current.wordNumber()].kind();
			allowBreak =
				((elementKind != TextElement::WORD_ELEMENT) || (previousKind == TextElement::WORD_ELEMENT)) &&
				(elementKind != TextElement::IMAGE_ELEMENT) &&
				(elementKind != TextElement::CONTROL_ELEMENT);
		}
		if (allowBreak) {
			info.IsVisible = isVisible;
			info.Width = newWidth;
			info.Height = std::max(info.Height, newHeight);
			info.Descent = std::max(info.Descent, newDescent);
			info.End = current;
			storedStyle = myStyle.style();
			info.SpaceCounter = internalSpaceCounter;
			removeLastSpace = !wordOccured && (info.SpaceCounter > 0);
		}
	} while (!current.equalWordNumber(end));

	if (!current.equalWordNumber(end) &&
		 ZLTextStyleCollection::instance().baseStyle().AutoHyphenationOption.value() &&
		 myStyle.style()->allowHyphenations()) {
		const TextElement &element = paragraphCursor[current.wordNumber()];
		if (element.kind() == TextElement::WORD_ELEMENT) {
			newWidth -= myStyle.elementWidth(element, current.charNumber());
			const ZLTextWord &word = (ZLTextWord&)element;
			int spaceLeft = maxWidth - newWidth;
			if ((word.Length > 3) && (spaceLeft > 2 * myStyle.context().spaceWidth())) {
				ZLUnicodeUtil::Ucs2String ucs2string;
				ZLUnicodeUtil::utf8ToUcs2(ucs2string, word.Data, word.Size);
				ZLTextHyphenationInfo hyphenationInfo = ZLTextHyphenator::instance().info(word);
				int hyphenationPosition = word.Length - 1;
				int subwordWidth = 0;
				for (; hyphenationPosition > 0; --hyphenationPosition) {
					if (hyphenationInfo.isHyphenationPossible(hyphenationPosition)) {
						subwordWidth = myStyle.wordWidth(word, 0, hyphenationPosition, ucs2string[hyphenationPosition - 1] != '-');
						if (subwordWidth <= spaceLeft) {
							break;
						}
					}
				}
				if (hyphenationPosition > 0) {
					info.IsVisible = true;
					info.Width = newWidth + subwordWidth;
					info.Height = std::max(info.Height, newHeight);
					info.Descent = std::max(info.Descent, newDescent);
					info.End = current;
					storedStyle = myStyle.style();
					info.SpaceCounter = internalSpaceCounter;
					removeLastSpace = false;
					info.End.setCharNumber(hyphenationPosition);
				}
			}
		}
	}

	if (removeLastSpace) {
		info.Width -= lastSpaceWidth;
		--info.SpaceCounter;
	}

	myStyle.setStyle(storedStyle);

	if (isFirstLine) {
		info.Height += info.StartStyle->spaceBefore();
	}
	if (info.End.isEndOfParagraph()) {
		info.VSpaceAfter = myStyle.style()->spaceAfter();
	}

	if (!info.End.equalWordNumber(end) || end.isEndOfParagraph()) {
		myLineInfoCache.insert(infoPtr);
	}

	return infoPtr;
}