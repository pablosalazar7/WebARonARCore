#include "config.h"

#define WIN32_COMPILE_HACK

#include <stdio.h>
#include <stdlib.h>
#include "NodeImpl.h"
#include "KWQLineEdit.h"
#include "Font.h"
#include "KWQFileButton.h"
#include "KWQTextEdit.h"
#include "KWQComboBox.h"
#include "IntPoint.h"
#include "Widget.h"
#include "GraphicsContext.h"
#include "KWQSlider.h"
#include "Cursor.h"
#include "loader.h"
#include "FrameView.h"
#include "KWQScrollBar.h"
#include "KWQKJavaAppletWidget.h"
#include "KWQScrollBar.h"
#include "Path.h"
#include "MouseEvent.h"
#include "KWQKCookieJar.h"
#include "Screen.h"
#include "History.h"
#include "KWQKLocale.h"
#include "PlugInInfoStore.h"
#include "render_theme.h"
#include "FrameWin.h"
#include "BrowserExtensionWin.h"
#include "TransferJob.h"
#include "RenderThemeWin.h"
#include "FontRenderer.h"

using namespace WebCore;

static void notImplemented() { puts("Not yet implemented"); _CrtDbgBreak(); }

IntPoint FrameView::viewportToGlobal(IntPoint const&) const { notImplemented(); return IntPoint(); }
void FrameView::updateBorder() { notImplemented(); }
bool FrameView::isFrameView() const { notImplemented(); return 0; }
Widget* FrameView::topLevelWidget() const { notImplemented(); return 0; }

QScrollBar::~QScrollBar() { notImplemented(); }
void QScrollBar::setSteps(int,int) { notImplemented(); }
bool QScrollBar::scroll(KWQScrollDirection,KWQScrollGranularity,float) { notImplemented(); return 0; }
QScrollBar::QScrollBar(ScrollBarOrientation) { notImplemented(); }
bool QScrollBar::setValue(int) { notImplemented(); return 0; }
void QScrollBar::setKnobProportion(int,int) { notImplemented(); }

QListBox::QListBox() { notImplemented(); }
void QListBox::setSelected(int,bool) { notImplemented(); }
IntSize QListBox::sizeForNumberOfLines(int) const { notImplemented(); return IntSize(); }
bool QListBox::isSelected(int) const { notImplemented(); return 0; }
void QListBox::appendItem(QString const&,KWQListBoxItemType,bool) { notImplemented(); }
void QListBox::doneAppendingItems() { notImplemented(); }

int ScrollView::scrollXOffset() const { notImplemented(); return 0; }
Widget::FocusPolicy QComboBox::focusPolicy() const { notImplemented(); return NoFocus; }

QTextEdit::QTextEdit(Widget*) { notImplemented(); }
QTextEdit::~QTextEdit() { notImplemented(); }
String QTextEdit::textWithHardLineBreaks() const { notImplemented(); return String(); }
IntSize QTextEdit::sizeWithColumnsAndRows(int,int) const { notImplemented(); return IntSize(); }
void QTextEdit::setText(String const&) { notImplemented(); }
void QTextEdit::setColors(Color const&,Color const&) { notImplemented(); }
void QTextEdit::setFont(WebCore::Font const&) { notImplemented(); }
void QTextEdit::setWritingDirection(enum WebCore::TextDirection) { notImplemented(); }
bool QTextEdit::checksDescendantsForFocus() const { notImplemented(); return false; }
int QTextEdit::selectionStart() { notImplemented(); return 0; }
bool QTextEdit::hasSelectedText() const { notImplemented(); return 0; }
int QTextEdit::selectionEnd() { notImplemented(); return 0; }
void QTextEdit::setScrollBarModes(ScrollBarMode,ScrollBarMode) { notImplemented(); }
void QTextEdit::setReadOnly(bool) { notImplemented(); }
void QTextEdit::selectAll() { notImplemented(); }
void QTextEdit::setDisabled(bool) { notImplemented(); }
void QTextEdit::setLineHeight(int) { notImplemented(); }
void QTextEdit::setSelectionStart(int) { notImplemented(); }
void QTextEdit::setCursorPosition(int,int) { notImplemented(); }
String QTextEdit::text() const { notImplemented(); return String(); }
void QTextEdit::setWordWrap(QTextEdit::WrapStyle) { notImplemented(); }
void QTextEdit::setAlignment(HorizontalAlignment) { notImplemented(); }
void QTextEdit::setSelectionEnd(int) { notImplemented(); }
void QTextEdit::getCursorPosition(int*,int*) const { notImplemented(); }
void QTextEdit::setSelectionRange(int,int) { notImplemented(); }

void QComboBox::populate() { notImplemented(); }

void Widget::enableFlushDrawing() { notImplemented(); }
bool Widget::isEnabled() const { notImplemented(); return 0; }
Widget::FocusPolicy Widget::focusPolicy() const { notImplemented(); return NoFocus; }
void Widget::disableFlushDrawing() { notImplemented(); }
void Widget::setIsSelected(bool) { notImplemented(); }
void Widget::lockDrawingFocus() { notImplemented(); }
void Widget::unlockDrawingFocus() { notImplemented(); }
void Widget::setDrawingAlpha(float) { notImplemented(); }

KJavaAppletWidget::KJavaAppletWidget(IntSize const&,Frame*,KXMLCore::HashMap<String,String> const&) { notImplemented(); }

int ScrollView::scrollYOffset() const { notImplemented(); return 0; }
void ScrollView::scrollPointRecursively(int,int) { notImplemented(); }
void ScrollView::scrollBy(int,int) { notImplemented(); }

void QLineEdit::selectAll() { notImplemented(); }
void QLineEdit::addSearchResult() { notImplemented(); }
int QLineEdit::selectionStart() const { notImplemented(); return 0; }
bool QLineEdit::hasSelectedText() const { notImplemented(); return 0; }
QString QLineEdit::selectedText() const { notImplemented(); return QString(); }
void QLineEdit::setAutoSaveName(String const&) { notImplemented(); }
bool QLineEdit::checksDescendantsForFocus() const { notImplemented(); return false; }
void QLineEdit::setSelection(int,int) { notImplemented(); }
void QLineEdit::setMaxResults(int) { notImplemented(); }
bool QLineEdit::edited() const { notImplemented(); return 0; }

GraphicsContext::GraphicsContext() { notImplemented(); }
void GraphicsContext::addFocusRingRect(int,int,int,int) { notImplemented(); }
void GraphicsContext::clearFocusRing() { notImplemented(); }
void GraphicsContext::drawLineForMisspelling(int,int,int) { notImplemented(); }
void GraphicsContext::drawFocusRing(Color const&) { notImplemented(); }
Color GraphicsContext::selectedTextBackgroundColor() const { notImplemented(); return Color(); }
void GraphicsContext::drawHighlightForText(int,int,int,int,int,QChar const*,int,int,int,int,Color const&,TextDirection,bool,int,int,bool) { notImplemented(); }
int GraphicsContext::misspellingLineThickness() const { notImplemented(); return 0; }
void GraphicsContext::addRoundedRectClip(IntRect const&,IntSize const&,IntSize const&,IntSize const&,IntSize const&) { notImplemented(); }
void GraphicsContext::initFocusRing(int,int) { notImplemented(); }

QSlider::QSlider() { notImplemented(); }
IntSize QSlider::sizeHint() const { notImplemented(); return IntSize(); }
void QSlider::setValue(double) { notImplemented(); }
void QSlider::setMaxValue(double) { notImplemented(); }
void QSlider::setMinValue(double) { notImplemented(); }
QSlider::~QSlider() { notImplemented(); }
void QSlider::setFont(WebCore::Font const&) { notImplemented(); }
double QSlider::value() const { notImplemented(); return 0; }

void QListBox::setFont(WebCore::Font const&) { notImplemented(); }
QListBox::~QListBox() { notImplemented(); }
void QListBox::setSelectionMode(QListBox::SelectionMode) { notImplemented(); }
void QListBox::setWritingDirection(TextDirection) { notImplemented(); }
void QListBox::setEnabled(bool) { notImplemented(); }
void QListBox::clear() { notImplemented(); }
bool QListBox::checksDescendantsForFocus() const { notImplemented(); return 0; }

KWQFileButton::KWQFileButton(Frame*) { notImplemented(); }
void KWQFileButton::click(bool) { notImplemented(); }
IntSize KWQFileButton::sizeForCharacterWidth(int) const { notImplemented(); return IntSize(); }
Widget::FocusPolicy KWQFileButton::focusPolicy() const { notImplemented(); return NoFocus; }
WebCore::IntRect KWQFileButton::frameGeometry() const { notImplemented(); return IntRect(); }
void KWQFileButton::setFilename(QString const&) { notImplemented(); }
int KWQFileButton::baselinePosition(int) const { notImplemented(); return 0; }
void KWQFileButton::setFrameGeometry(WebCore::IntRect const&) { notImplemented(); }
KWQFileButton::~KWQFileButton() { notImplemented(); }

Widget::FocusPolicy QTextEdit::focusPolicy() const { notImplemented(); return NoFocus; }
Widget::FocusPolicy QSlider::focusPolicy() const { notImplemented(); return NoFocus; }
Widget::FocusPolicy QListBox::focusPolicy() const { notImplemented(); return NoFocus; }
Widget::FocusPolicy QLineEdit::focusPolicy() const { notImplemented(); return NoFocus; }

static Cursor localCursor;
Cursor::Cursor(Image*) { notImplemented(); }
const Cursor& WebCore::moveCursor() { notImplemented(); return localCursor; }

MouseEvent::MouseEvent() { notImplemented(); }
QString searchableIndexIntroduction() { notImplemented(); return QString(); }

void KWQKCookieJar::setCookie(KURL const&,KURL const&,QString const&) { notImplemented(); }
QString KWQKCookieJar::cookie(KURL const&) { notImplemented(); return QString(); }
bool KWQKCookieJar::cookieEnabled() { notImplemented(); return false; }

IntRect WebCore::screenRect(Widget*) { notImplemented(); return IntRect(); }
int WebCore::screenDepth(Widget*) { notImplemented(); return 96; }
IntRect WebCore::usableScreenRect(Widget*) { notImplemented(); return IntRect(0,0,800,600); }

int KWQFindNextSentenceFromIndex(QChar const*,int,int,bool) { notImplemented(); return 0; }
void KWQFindSentenceBoundary(QChar const*,int,int,int*,int*) { notImplemented(); }
int KWQFindNextWordFromIndex(QChar const*,int,int,bool) { notImplemented(); return 0; }
void KWQFindWordBoundary(QChar const*,int,int,int*,int*) { notImplemented(); }

PluginInfo*PlugInInfoStore::createPluginInfoForPluginAtIndex(unsigned) { notImplemented(); return 0;}
unsigned PlugInInfoStore::pluginCount() const { notImplemented(); return 0; }
void WebCore::refreshPlugins(bool) { notImplemented(); }

Array<char> KWQServeSynchronousRequest(Loader*,DocLoader*,TransferJob*,KURL&,QString&) { notImplemented(); return 0; }

void FrameWin::respondToChangedContents() { notImplemented(); }
void FrameWin::unfocusWindow() { notImplemented(); }
bool FrameWin::locationbarVisible() { notImplemented(); return 0; }
void FrameWin::respondToChangedSelection(WebCore::SelectionController const&,bool) { notImplemented(); }
void FrameWin::issueRedoCommand(void) { notImplemented(); }
KJS::Bindings::Instance* FrameWin::getObjectInstanceForWidget(Widget *) { notImplemented(); return 0; }
KJS::Bindings::Instance* FrameWin::getEmbedInstanceForWidget(Widget *) { notImplemented(); return 0; }
bool FrameWin::canRedo() const { notImplemented(); return 0; }
bool FrameWin::canUndo() const { notImplemented(); return 0; }
bool FrameWin::runJavaScriptPrompt(String const&,String const&,String &) { notImplemented(); return 0; }
void FrameWin::recordFormValue(QString const&,QString const&,WebCore::HTMLFormElementImpl*) { notImplemented(); }
void FrameWin::registerCommandForRedo(WebCore::EditCommandPtr const&) { notImplemented(); }
void FrameWin::runJavaScriptAlert(String const&) { notImplemented(); }
bool FrameWin::runJavaScriptConfirm(String const&) { notImplemented(); return 0; }
bool FrameWin::openURL(KURL const&) { notImplemented(); return 0; }
void FrameWin::print() { notImplemented(); }
KJS::Bindings::Instance* FrameWin::getAppletInstanceForWidget(Widget*) { notImplemented(); return 0; }
bool FrameWin::passMouseDownEventToWidget(Widget*) { notImplemented(); return 0; }
void FrameWin::registerCommandForUndo(WebCore::EditCommandPtr const&) { notImplemented(); }
void FrameWin::issueCutCommand() { notImplemented(); }
void FrameWin::issueCopyCommand() { notImplemented(); }
void FrameWin::openURLRequest(KURL const&,struct WebCore::URLArgs const&) { notImplemented(); }
bool FrameWin::passWheelEventToChildWidget(NodeImpl*) { notImplemented(); return 0; }
void FrameWin::issueUndoCommand() { notImplemented(); }
QString FrameWin::mimeTypeForFileName(QString const&) const { notImplemented(); return QString(); }
void FrameWin::clearRecordedFormValues() { notImplemented(); }
void FrameWin::issuePasteCommand() { notImplemented(); }
void FrameWin::scheduleClose() { notImplemented(); }
void FrameWin::markMisspellingsInAdjacentWords(WebCore::VisiblePosition const&) { notImplemented(); }
void FrameWin::markMisspellings(WebCore::SelectionController const&) { notImplemented(); }
bool FrameWin::menubarVisible() { notImplemented(); return 0; }
bool FrameWin::personalbarVisible() { notImplemented(); return 0; }
bool FrameWin::statusbarVisible() { notImplemented(); return 0; }
bool FrameWin::toolbarVisible() { notImplemented(); return 0; }
void FrameWin::issueTransposeCommand() { notImplemented(); }
void FrameWin::submitForm(KURL const&,struct WebCore::URLArgs const&) { notImplemented(); }
bool FrameWin::canPaste() const { notImplemented(); return 0; }
QString FrameWin::incomingReferrer() const { notImplemented(); return QString(); }
enum WebCore::ObjectContentType FrameWin::objectContentType(KURL const&,QString const&) { notImplemented(); return (ObjectContentType)0; }
bool FrameWin::canGoBackOrForward(int) const { notImplemented(); return 0; }
void FrameWin::issuePasteAndMatchStyleCommand() { notImplemented(); }
Plugin* FrameWin::createPlugin(KURL const&,QStringList const&,QStringList const&,QString const&) { notImplemented(); return 0; }

void BrowserExtensionWin::openURLRequest(KURL const&,struct WebCore::URLArgs const&) { notImplemented(); }
int BrowserExtensionWin::getHistoryLength() { notImplemented(); return 0; }
bool BrowserExtensionWin::canRunModal() { notImplemented(); return 0; }
void BrowserExtensionWin::openURLNotify() { notImplemented(); }
void BrowserExtensionWin::createNewWindow(KURL const&,struct WebCore::URLArgs const&,struct WebCore::WindowArgs const&,Frame*&) { notImplemented(); }
bool BrowserExtensionWin::canRunModalNow() { notImplemented(); return 0; }
void BrowserExtensionWin::runModal() { notImplemented(); }
void BrowserExtensionWin::goBackOrForward(int) { notImplemented(); }
void BrowserExtensionWin::setIconURL(KURL const&) { notImplemented(); }
void BrowserExtensionWin::createNewWindow(KURL const&,struct WebCore::URLArgs const&) { notImplemented(); }

void WebCore::TransferJob::retrieveCharset() const { notImplemented(); }
void WebCore::TransferJob::assembleResponseHeaders() const { notImplemented(); }

IntRect Font::selectionRectForText(int, int, int, int, int, const QChar*, int, int, int, int, bool, bool, int, int) const { notImplemented(); return IntRect(); }
void Font::drawHighlightForText(WebCore::GraphicsContext*, int, int, int, int, int, const QChar*, int, int, int, int, WebCore::TextDirection, bool, int, int, WebCore::Color) const { notImplemented(); }



// Completely empty stubs (mostly to allow DRT to run):
bool WebCore::historyContains(QString const&) { return false; }
QString submitButtonDefaultLabel() { return "Submit"; }
QString inputElementAltText() { return QString(); }
QString resetButtonDefaultLabel() { return "Reset"; }
QString KLocale::language() { return "en"; }

void FrameWin::restoreDocumentState() { }
void FrameWin::partClearedInBegin() { }
void FrameWin::createEmptyDocument() { }
QString FrameWin::overrideMediaType() const { return QString(); }
void FrameWin::setTitle(String const&) { }
void FrameWin::handledOnloadEvents() { }
RangeImpl* FrameWin::markedTextRange() const { return 0; }
bool FrameWin::passSubframeEventToSubframe(WebCore::MouseEventWithHitTestResults&) { return false; }
bool FrameWin::lastEventIsMouseUp() const { return false; }
void FrameWin::addMessageToConsole(String const&,unsigned int,String const&) { }
bool FrameWin::shouldChangeSelection(SelectionController const&,SelectionController const&,WebCore::EAffinity,bool) const { return 0; }
static int frameNumber = 0;
String FrameWin::generateFrameName() { return QString::number(frameNumber++); }
Frame* FrameWin::createFrame(KURL const&,QString const&,RenderPart*,String const&) { return 0; }
void FrameWin::saveDocumentState(void) { }
void FrameWin::clearUndoRedoOperations(void) {}
void FrameWin::urlSelected(KURL const&,struct WebCore::URLArgs const&) { }

BrowserExtensionWin::BrowserExtensionWin(WebCore::Frame*) { }
void BrowserExtensionWin::setTypedIconURL(KURL const&,QString const&) { }

bool KWQCheckIfReloading(WebCore::DocLoader*) { return false; }
void KWQCheckCacheObjectStatus(DocLoader*, CachedObject*) { }

void Widget::setEnabled(bool) { }
void Widget::paint(GraphicsContext*,IntRect const&) { }

void ScrollView::resizeContents(int w,int h) { resize(w, h); }
WebCore::ScrollBarMode ScrollView::hScrollBarMode() const { return ScrollBarAlwaysOff; }
WebCore::ScrollBarMode ScrollView::vScrollBarMode() const { return ScrollBarAlwaysOff; }
void ScrollView::suppressScrollBars(bool,bool) { }
void ScrollView::setHScrollBarMode(ScrollBarMode) { }
void ScrollView::setVScrollBarMode(ScrollBarMode) { }
void ScrollView::setScrollBarsMode(ScrollBarMode) { }
int ScrollView::visibleHeight() const { return height(); }
int ScrollView::visibleWidth() const { return width(); }
void ScrollView::setContentsPos(int x,int y) { move(x, y); }
int ScrollView::contentsX() const { return x(); }
int ScrollView::contentsY() const { return y(); }
int ScrollView::contentsHeight() const { return height(); }
int ScrollView::contentsWidth() const { return width(); }
void ScrollView::viewportToContents(int x1, int y1, int& x2, int& y2) { x2 = x1; y2 = y1; }
void ScrollView::setStaticBackground(bool) { }
void ScrollView::addChild(Widget*,int,int) { }
void ScrollView::removeChild(Widget*) { }
bool ScrollView::inWindow() const { return true; }

float Font::floatWidth(QChar const*, int, int, int len, int, int) const { return len * 10; }
int Font::descent() const { return 10; }
float Font::xHeight() const { return 10; }
int Font::ascent() const { return 10; }
int Font::lineSpacing() const { return 10; }
bool Font::isFixedPitch() const { return true; }
void Font::drawText(WebCore::GraphicsContext *, int, int, int, int, const QChar*, int, int, int, int, WebCore::TextDirection, bool, int, int, WebCore::Color) const { }
int Font::checkSelectionPoint(const QChar*, int, int, int, int, int, int, int, WebCore::TextDirection, bool, bool) const { return 0; }

FontRenderer::FontRenderer() { }
void FontRenderer::update(FontDescription const &) { }

static WebCore::Font localFont;
Font const& GraphicsContext::font() const { return localFont; }
void GraphicsContext::setFont(Font const&) { }
void GraphicsContext::drawText(int,int,int,int,int,int,int,QString const&) { }
void GraphicsContext::drawText(int,int,int,int,QChar const*,int,int,int,int,Color const&,TextDirection,bool,int,int,bool) { }
void GraphicsContext::drawLineForText(int,int,int,int) { }
void GraphicsContext::setShadow(int,int,int,Color const&) { }
void GraphicsContext::clearShadow() { }
void GraphicsContext::beginTransparencyLayer(float) { }
void GraphicsContext::endTransparencyLayer() { }

Path::Path(){ }
Path::~Path(){ }
Path::Path(const Path&){ }
Path::Path(const IntRect&, Type){ }
Path::Path(const IntPointArray&){ }
bool Path::contains(const IntPoint&) const{ return false; }
void Path::translate(int, int){ }
IntRect Path::boundingRect() const { return IntRect(); }

Path& Path::operator=(const Path&){ return*this; }

bool RenderThemeWin::paintCheckbox(RenderObject*, const RenderObject::PaintInfo&, const IntRect&) { return false; }
bool RenderThemeWin::paintRadio(RenderObject*, const RenderObject::PaintInfo&, const IntRect&) { return false; }
bool RenderThemeWin::paintButton(RenderObject*, const RenderObject::PaintInfo&, const IntRect&) { return false; }
bool RenderThemeWin::paintTextField(RenderObject*, const RenderObject::PaintInfo&, const IntRect&) { return false; }

QLineEdit::QLineEdit(QLineEdit::Type) { }
QLineEdit::~QLineEdit() { }
void QLineEdit::setFont(WebCore::Font const&) { }
void QLineEdit::setAlignment(HorizontalAlignment) { }
void QLineEdit::setWritingDirection(TextDirection) { }
int QLineEdit::maxLength() const { return 0; }
void QLineEdit::setMaxLength(int) { }
String QLineEdit::text() const { return String(); }
void QLineEdit::setText(String const&) { }
int QLineEdit::cursorPosition() const { return 0; }
void QLineEdit::setCursorPosition(int) { }
void QLineEdit::setEdited(bool) { }
void QLineEdit::setReadOnly(bool) { }
void QLineEdit::setPlaceholderString(String const&) { }
void QLineEdit::setColors(Color const&,Color const&) { }
IntSize QLineEdit::sizeForCharacterWidth(int) const { return IntSize(); }
int QLineEdit::baselinePosition(int) const { return 0; }
void QLineEdit::setLiveSearch(bool) { }

QComboBox::QComboBox() { }
QComboBox::~QComboBox() { }
void QComboBox::setFont(WebCore::Font const&) { }
int QComboBox::baselinePosition(int) const { return 0; }
void QComboBox::setWritingDirection(TextDirection) { }
void QComboBox::clear() { }
void QComboBox::appendItem(QString const&,KWQListBoxItemType,bool) { }
void QComboBox::setCurrentItem(int) { }
IntSize QComboBox::sizeHint() const { return IntSize(); }
IntRect QComboBox::frameGeometry() const { return IntRect(); }
void QComboBox::setFrameGeometry(IntRect const&) { }

