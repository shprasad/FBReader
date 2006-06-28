/*
 * Copyright (C) 2004-2006 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005, 2006 Mikhail Sobolev <mss@mawhrin.net>
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

#ifndef __ZLAPPLICATION_H__
#define __ZLAPPLICATION_H__

#include <string>
#include <vector>
#include <map>

#include <abstract/shared_ptr.h>
#include <abstract/ZLOptions.h>

class ZLView;
class ZLViewWidget;

class ZLApplication {

public:
	static const std::string MouseScrollUpKey;
	static const std::string MouseScrollDownKey;

public:
	ZLIntegerOption RotationAngleOption;
	ZLIntegerOption AngleStateOption;

  ZLBooleanOption KeyboardControlOption;

public:

	class Action {

	public:
		virtual ~Action();
		virtual bool isVisible();
		virtual bool isEnabled();
		void checkAndRun();

	protected:
		virtual void run() = 0;
	};

protected:
	class RotationAction : public Action {

	public:
		RotationAction(ZLApplication &application);
		bool isVisible();
		void run();

	private:
		ZLApplication &myApplication;
	};
	friend class RotationAction;
	
	class FullscreenAction : public Action {

	public:
		FullscreenAction(ZLApplication &application, bool toggle);
		bool isVisible();
		void run();

	private:
		ZLApplication &myApplication;
		bool myIsToggle;
	};
	friend class FullscreenAction;
	
public:

	class Toolbar {

	public:
		class Item {

		public:
			Item();
			virtual ~Item();

			virtual bool isButton() const = 0;
			bool isSeparator() const;

		friend class Toolbar;
		};
	
		class ButtonItem : public Item {

		public:
			ButtonItem(int actionId, const std::string &iconName);

			bool isButton() const;

			int actionId() const;
			const std::string &iconName() const;

		private:
			const int myActionId;
			const std::string myIconName;

		friend class Toolbar;
		};
	
		class SeparatorItem : public Item {

		public:
			bool isButton() const;
		};

	public:
		typedef shared_ptr<Item> ItemPtr;
		typedef std::vector<ItemPtr> ItemVector;

		void addButton(int actionId, const std::string &iconName);
		void addSeparator();

		const ItemVector &items() const;

	private:
		ItemVector myItems;

	friend class ZLApplication;
	};

	class Menu {

	public:
		class Item {

		public:
			enum ItemType {
				MENU_ITEM,
				SUBMENU_ITEM,
				SEPARATOR_ITEM
			};

			virtual ~Item();

			ItemType type() const;

		protected:
			Item(ItemType type);

		private:
			const ItemType myType;
		};

		typedef shared_ptr<Item> ItemPtr;
		typedef std::vector<ItemPtr> ItemVector;

		void addItem(const std::string &itemName, int actionId);
		void addSeparator();
		Menu& addSubmenu(const std::string &menuName);

		const ItemVector &items() const;

		virtual ~Menu();

	protected:
		Menu();

	private:
		ItemVector myItems;

	friend class ZLApplication;
	};

	class Menubar : public Menu {

	public:
		class MenuItem : public Menu::Item {

		public:
			MenuItem(const std::string &name, int actionId);

			const std::string &name() const;
			int actionId() const;

		private:
			const std::string myName;
			int myActionId;
		};

		class SubMenuItem : public Menu::Item, public Menu {

		public:
			SubMenuItem(const std::string &menuName);

			const std::string &menuName() const;

		private:
			const std::string myMenuName;
		};

		class SeparatorItem : public Menu::Item {

		public:
			SeparatorItem();
		};

	public:
		Menubar();

	friend class ZLApplication;
	};

protected:
	ZLApplication(const std::string &name);

	void addAction(int actionId, shared_ptr<Action> action);
	void setView(ZLView *view);
	ZLView *currentView();

	void setFullscreen(bool fullscreen);
	bool isFullscreen() const;
	void resetWindowCaption();
	void quit();

public:
	virtual void initWindow();
	// TODO: remove
	void repaintView();
  void grabAllKeys(bool grab);
	bool isFullKeyboardControlSupported() const;

public:
	virtual ~ZLApplication();

	const std::string &name() const;
	shared_ptr<Action> action(int actionId) const;
	bool isActionVisible(int actionId) const;
	bool isActionEnabled(int actionId) const;
	void doAction(int actionId);
	// TODO: move implementation from FBReader
  virtual void doActionByKey(const std::string &key) = 0;
	virtual bool closeView();
	virtual void openFile(const std::string &fileName);

	Toolbar &toolbar();
	Menubar &menubar();
	const Toolbar &toolbar() const;
	const Menubar &menubar() const;

	void refreshWindow();

private:
	const std::string myName;

protected:
	ZLViewWidget *myViewWidget;

private:
	std::map<int,shared_ptr<Action> > myActionMap;
	Toolbar myToolbar;
	Menubar myMenubar;
	class ZLApplicationWindow *myWindow;

friend class ZLApplicationWindow;
};

class ZLApplicationWindow {

protected:
	ZLApplicationWindow(ZLApplication *application);

public:
	ZLApplication &application() const;

protected:
	void init();
	virtual ZLViewWidget *createViewWidget() = 0;
	virtual void addToolbarItem(ZLApplication::Toolbar::ItemPtr item) = 0;
	// TODO: replace with pure virtual method when available for all platforms
	virtual void addMenubarItem(ZLApplication::Menubar::ItemPtr) {}

	virtual void refresh() = 0;

	virtual void close() = 0;

	virtual void setCaption(const std::string &caption) = 0;

	virtual bool isFullKeyboardControlSupported() const = 0;
  virtual void grabAllKeys(bool grab) = 0;

	virtual void setFullscreen(bool fullscreen) = 0;
	virtual bool isFullscreen() const = 0;

public:
	virtual ~ZLApplicationWindow();

private:
	ZLApplication *myApplication;

friend class ZLApplication;
};

inline ZLApplication::Toolbar &ZLApplication::toolbar() { return myToolbar; }
inline ZLApplication::Menubar &ZLApplication::menubar() { return myMenubar; }
inline const ZLApplication::Toolbar &ZLApplication::toolbar() const { return myToolbar; }
inline const ZLApplication::Menubar &ZLApplication::menubar() const { return myMenubar; }

inline void ZLApplication::refreshWindow() {
	if (myWindow != 0) {
		myWindow->refresh();
	}
}
inline bool ZLApplication::isFullKeyboardControlSupported() const {
	return (myWindow != 0) && myWindow->isFullKeyboardControlSupported();
}
inline void ZLApplication::grabAllKeys(bool grab) {
	if (myWindow != 0) {
		myWindow->grabAllKeys(grab);
	}
}
inline bool ZLApplication::isFullscreen() const {
	return (myWindow != 0) && myWindow->isFullscreen();
}
inline void ZLApplication::setFullscreen(bool fullscreen) {
	if (myWindow != 0) {
		myWindow->setFullscreen(fullscreen);
	}
}
inline void ZLApplication::quit() {
	if (myWindow != 0) {
		myWindow->close();
	}
}
inline const std::string &ZLApplication::name() const { return myName; }

inline ZLApplicationWindow::~ZLApplicationWindow() {}
inline ZLApplication &ZLApplicationWindow::application() const { return *myApplication; }

inline const ZLApplication::Toolbar::ItemVector &ZLApplication::Toolbar::items() const { return myItems; }

inline ZLApplication::Toolbar::Item::Item() {}
inline ZLApplication::Toolbar::Item::~Item() {}
inline bool ZLApplication::Toolbar::Item::isSeparator() const { return !isButton(); }

inline ZLApplication::Toolbar::ButtonItem::ButtonItem(int actionId, const std::string &iconName) : myActionId(actionId), myIconName(iconName) {}
inline bool ZLApplication::Toolbar::ButtonItem::isButton() const { return true; }
inline int ZLApplication::Toolbar::ButtonItem::actionId() const { return myActionId; }
inline const std::string &ZLApplication::Toolbar::ButtonItem::iconName() const { return myIconName; }

inline bool ZLApplication::Toolbar::SeparatorItem::isButton() const { return false; }

inline ZLApplication::Menu::Menu() {}
inline ZLApplication::Menu::~Menu() {}
inline const ZLApplication::Menu::ItemVector &ZLApplication::Menu::items() const { return myItems; }

inline ZLApplication::Menu::Item::Item(ItemType type): myType(type) {}
inline ZLApplication::Menu::Item::~Item() {}
inline ZLApplication::Menu::Item::ItemType ZLApplication::Menubar::Item::type() const { return myType; }

inline ZLApplication::Menubar::Menubar() {}

inline ZLApplication::Menubar::MenuItem::MenuItem(const std::string& name, int actionId) : Item(MENU_ITEM), myName(name), myActionId(actionId) {}
inline const std::string& ZLApplication::Menubar::MenuItem::name() const { return myName; }
inline int ZLApplication::Menubar::MenuItem::actionId() const { return myActionId; }

inline ZLApplication::Menubar::SubMenuItem::SubMenuItem(const std::string &menuName) : Menu::Item(SUBMENU_ITEM), myMenuName(menuName) {}
inline const std::string &ZLApplication::Menubar::SubMenuItem::menuName() const { return myMenuName; }

inline ZLApplication::Menubar::SeparatorItem::SeparatorItem(void) : Item(SEPARATOR_ITEM) {}

#endif /* __ZLAPPLICATION_H__ */

// vim:noet:ts=2:sw=2
