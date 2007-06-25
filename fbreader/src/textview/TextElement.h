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

#ifndef __TEXTELEMENT_H__
#define __TEXTELEMENT_H__

#include <ZLImageManager.h>

#include "../textmodel/TextKind.h"
#include "../textmodel/Paragraph.h"

class TextElement {

protected:
	TextElement();

public:
	virtual ~TextElement();

	enum Kind {
		WORD_ELEMENT,
		IMAGE_ELEMENT,
		CONTROL_ELEMENT,
		FORCED_CONTROL_ELEMENT,
		INDENT_ELEMENT,
		HSPACE_ELEMENT,
		FIXED_HSPACE_ELEMENT,
		BEFORE_PARAGRAPH_ELEMENT,
		AFTER_PARAGRAPH_ELEMENT,
		EMPTY_LINE_ELEMENT,
	};

	virtual Kind kind() const = 0;

private:
	// assignment and copy constructor are disabled
	TextElement(const TextElement&);
	TextElement &operator = (const TextElement&);
};

class ImageElement : public TextElement {

public:
	ImageElement(const std::string &id, const shared_ptr<ZLImageData> image);
	~ImageElement();
	Kind kind() const;
	const ZLImageData &image() const;
	const std::string &id() const;

private:
	const std::string myId;
	const shared_ptr<ZLImageData> myImage;
};

class SpecialTextElement : public TextElement {

public:
	SpecialTextElement(Kind kind);
	~SpecialTextElement();
	Kind kind() const;

private:
	Kind myKind;
};

class ForcedControlElement : public TextElement {

public:
	ForcedControlElement(shared_ptr<ParagraphEntry> entry);
	~ForcedControlElement();
	Kind kind() const;
	const ForcedControlEntry &entry() const;

private:
	const shared_ptr<ParagraphEntry> myEntry;
};

class FixedHSpaceElement : public TextElement {

public:
	FixedHSpaceElement(unsigned char length);
	Kind kind() const;
	unsigned char length() const;

private:
	const unsigned char myLength;
};

class ControlElement : public TextElement {

private:
	ControlElement(shared_ptr<ParagraphEntry> entry);
	~ControlElement();

public:
	Kind kind() const;
	const ControlEntry &entry() const;
	TextKind textKind() const;
	bool isStart() const;

private:
	const shared_ptr<ParagraphEntry> myEntry;

friend class TextElementPool;
};

inline TextElement::TextElement() {}
inline TextElement::~TextElement() {}

inline ImageElement::ImageElement(const std::string &id, const shared_ptr<ZLImageData> image) : myId(id), myImage(image) {}
inline ImageElement::~ImageElement() {}
inline TextElement::Kind ImageElement::kind() const { return IMAGE_ELEMENT; };
inline const ZLImageData &ImageElement::image() const { return *myImage; }
inline const std::string &ImageElement::id() const { return myId; }

inline SpecialTextElement::SpecialTextElement(Kind kind) : myKind(kind) {}
inline SpecialTextElement::~SpecialTextElement() {}
inline TextElement::Kind SpecialTextElement::kind() const { return myKind; };

inline ForcedControlElement::ForcedControlElement(const shared_ptr<ParagraphEntry> entry) : myEntry(entry) {}
inline ForcedControlElement::~ForcedControlElement() {}
inline TextElement::Kind ForcedControlElement::kind() const { return FORCED_CONTROL_ELEMENT; };
inline const ForcedControlEntry &ForcedControlElement::entry() const { return (const ForcedControlEntry&)*myEntry; }

inline ControlElement::ControlElement(const shared_ptr<ParagraphEntry> entry) : myEntry(entry) {}
inline ControlElement::~ControlElement() {}
inline TextElement::Kind ControlElement::kind() const { return CONTROL_ELEMENT; };
inline const ControlEntry &ControlElement::entry() const { return (const ControlEntry&)*myEntry; }
inline TextKind ControlElement::textKind() const { return entry().kind(); }
inline bool ControlElement::isStart() const { return entry().isStart(); }

inline FixedHSpaceElement::FixedHSpaceElement(unsigned char length) : myLength(length) {}
inline TextElement::Kind FixedHSpaceElement::kind() const { return FIXED_HSPACE_ELEMENT; }
inline unsigned char FixedHSpaceElement::length() const { return myLength; }

#endif /* __TEXTELEMENT_H__ */
