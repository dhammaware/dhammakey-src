/*
 * SPDX-FileCopyrightText: 2026 Ujudhammo
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
 *
 * DhammaKey - a DhammaWare project
 * Homepage: https://dhammaware.org
 *
 * Copyright (C) 2026 Ujudhammo.
 *
 * TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
 * This software is offered as dana (a free gift) to serve the community, to
 * support study and research, and to help spread the Dhamma. You may use, read,
 * study, modify and contribute improvements to this source code for
 * noncommercial purposes.
 *
 * PROHIBITED: copying, packaging, redistributing or using this source code or
 * any derivative work for commercial purposes, for sale, or for any fee.
 *
 * The full, legally binding terms are in the LICENSE file at the project root.
 * Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.
 */

// Cửa sổ cài đặt DhammaKey — Qt 5.15 Widgets, bố cục kiểu Settings của Windows 11:
// danh sách mục bên trái, trang nội dung bên phải (không phải cuộn).
// 3 giao diện: Vừa (sáng), Tối, Cổ kính. Mọi thay đổi được lưu ngay.
#include <dhammakey_pali_keys.h>
#include <QApplication>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QFileInfo>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QTextBrowser>
#include <QFontMetrics>
#include <QtMath>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QRadialGradient>
#include <cmath>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <thread>

#include "dhammakey.h"
#include "config.h"
#include "net.h"

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM
#include <dwmapi.h>
#include "Globals.h"   // DK_REG_KEY — cùng chỗ lưu với bộ gõ
#include "Conflict.h"
#endif

// ---------------- Lưu / đọc cài đặt ----------------
namespace store {
#ifdef _WIN32
int get(const wchar_t* name, int def) {
    DWORD v = 0, sz = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, DK_REG_KEY, name, RRF_RT_REG_DWORD, nullptr, &v, &sz) != ERROR_SUCCESS)
        return def;
    return (int)v;
}
void set(const wchar_t* name, int value) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, DK_REG_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    DWORD v = (DWORD)value;
    RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
    RegCloseKey(k);
}
// Bộ chữ trên tray icon khi chưa chọn: máy ở Việt Nam dùng chữ Latinh (0), nước khác chữ bản địa (1).
// Cùng quy tắc với TrayGlyph() trong platform/windows/LangBar.cpp.
int defaultTrayGlyph() { return GetUserGeoID(GEOCLASS_NATION) == 251 ? 0 : 1; }
#else
int get(const wchar_t*, int def) { return def; }  // macOS / Linux: làm sau
void set(const wchar_t*, int) {}
int defaultTrayGlyph() { return 0; }
#endif
} // namespace store

// Ngôn ngữ giao diện (bước 1, 0.4.0): đọc từ registry, 0 = Tiếng Việt, 1 = English.
static bool uiEnglish() { return store::get(L"UiLang", 0) == 1; }

// ---------------- Giao diện (skin) ----------------
struct Theme {
    QString name;
    bool dark;
    QColor bg, nav, card, border, sep, text, hint, accent, accentText, ctrl, ctrlHover, ctrlBorder, navHover, navSel, warn;
    QString font, titleFont;
};

static Theme makeTheme(int i) {
    Theme t;
    if (i == 1) {  // Tối
        t = {QStringLiteral("Tối"), true,
             "#202020", "#1c1c1c", "#2b2b2b", "#1d1d1d", "#3a3a3a", "#ffffff", "#c5c5c5",
             "#4cc2ff", "#000000", "#373737", "#3f3f3f", "#4a4a4a", "#2d2d2d", "#383838", "#ff99a4",
             "Segoe UI", "Segoe UI"};
    } else if (i == 2) {  // Cổ kính: giấy bối diệp, mực nâu, màu y vàng nghệ
        t = {QStringLiteral("Cổ kính"), false,
             "#efe4cc", "#e6d8b8", "#faf4e4", "#d6c39b", "#e3d4b1", "#3b2a1a", "#7a6347",
             "#9c5a1a", "#fffaf0", "#f6eedb", "#efe3c8", "#c9b285", "#ddcca6", "#d4bf92", "#a8321e",
             "Cambria", "Cambria"};
    } else {  // Vừa (sáng)
        t = {QStringLiteral("Vừa"), false,
             "#f3f3f3", "#eeeeee", "#ffffff", "#e5e5e5", "#ececec", "#1b1b1b", "#616161",
             "#0067c0", "#ffffff", "#fbfbfb", "#f3f3f3", "#d6d6d6", "#e5e5e5", "#e0e0e0", "#c42b1c",
             "Segoe UI", "Segoe UI"};
    }
    return t;
}

#ifndef DK_COPYRIGHT_YEAR
#define DK_COPYRIGHT_YEAR "2026"
#endif
// Người giữ bản quyền (người dùng chốt 14/11/2026)
#ifndef DK_COPYRIGHT_HOLDER
#define DK_COPYRIGHT_HOLDER "Ujudhammo"
#endif
// Slogan chính thức (bộ nhận diện 1.5, viết lại 15/11/2026): gạch dài U+2013 có dấu cách hai bên,
// KHÔNG có dấu chấm cuối. Luôn đi kèm logo DhammaKey, luôn bằng tiếng Anh — KHÔNG bọc tr().
// Bản ASCII cho thuộc tính tệp (.rc, .wxs) ghi riêng: "Breaking barriers - typing beyond".
#ifndef DK_SLOGAN
#define DK_SLOGAN "Breaking barriers \u2013 typing beyond"
#endif
static Theme g_theme = makeTheme(0);
// Bản tiếng Anh của lời giới thiệu, bản quyền và dòng phiên bản (đã duyệt 17/11/2026).
// CHƯA DÙNG: sẽ nối vào khi giao diện Anh–Việt có ở 0.4.0. Giữ đồng bộ với bản tiếng Việt trong AboutWindow.
#define DK_ABOUT_EN_TEXT \
    "<p><b>DhammaKey™</b> is a free multilingual input method developed by <b>DhammaWare™</b>, aimed at the input needs of " \
    "scholarship, research, translation, education, Buddhist studies, and the use of international languages across many platforms.</p>" \
    "<p>The application gradually adds convenient typing for important scripts and languages such as Vietnamese, Pāli, " \
    "Sanskrit, Burmese (Myanmar), Thai, Chinese, Korean, Japanese, and Han-Nom, along with the traditional input methods of each country. " \
    "DhammaKey is designed to respect local standards, helping users work in line with the habits and input conventions of each language.</p>"
#define DK_ABOUT_EN_VERSION "DhammaKey™ · Version %1 · dāna"
#define DK_ABOUT_EN_LEGAL "© %1 %2<br>DhammaKey™, DhammaPāṭha™ and DhammaWare™ belong to DhammaWare."

// Màu slogan theo bộ nhận diện 1.5 (mẫu A): cùng màu chữ "Key" — tím nhạt #8C5A7C trên nền sáng,
// tím sương #D9C3D2 trên nền tối
static QColor sloganColor(const Theme& t) { return t.dark ? QColor("#D9C3D2") : QColor("#8C5A7C"); }
// Mẫu A: slogan dài đúng bằng chữ "DhammaKey" — tìm cỡ chữ (pixel) của phông gốc để vừa bề rộng đó
static QFont sloganFontForWidth(const QFont& base, int width) {
    QFont f(base);
    f.setWeight(QFont::Normal);
    f.setPixelSize(40);
    const qreal w40 = QFontMetricsF(f).horizontalAdvance(QStringLiteral(DK_SLOGAN));
    f.setPixelSize(qMax(7, int(40.0 * width / w40)));
    return f;
}
static QString sloganCss(const Theme& t) {   // cho style sheet (giữ độ trong)
    QColor c = sloganColor(t);
    return QStringLiteral("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

// Mũi tên ▾ cho ô chọn: vẽ ra tệp PNG tạm theo màu chữ của skin
static QString arrowImage(const QColor& c) {
    QString path = QDir::temp().filePath(QStringLiteral("dhammakey-arrow-%1.png").arg(c.name().mid(1)));
    if (!QFile::exists(path)) {
        QPixmap pm(24, 24);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({QPointF(6, 9), QPointF(12, 15), QPointF(18, 9)}));
        p.end();
        pm.save(path);
    }
    return QDir::fromNativeSeparators(path);
}

static QString styleSheetFor(const Theme& t) {
    QString s = QStringLiteral(R"(
        QWidget { font-family: "${font}"; font-size: 10pt; color: ${text}; }
        #root, #page, QStackedWidget, QScrollArea, #scrollBody { background: ${bg}; border: none; }
        #nav { background: ${nav}; border: none; outline: none; padding: 6px; }
        #nav::item { padding: 8px 10px; margin: 2px 0; border-radius: 5px; border-left: 3px solid transparent; }
        #nav::item:hover { background: ${navHover}; }
        #nav::item:selected { background: ${navSel}; color: ${text}; border-left: 3px solid ${accent}; }
        #navTitle { font-family: "${titleFont}"; font-size: 15pt; font-weight: 600; padding: 0; }
        #navSlogan { color: ${slogan}; padding: 0; }
        #sectionTitle { font-family: "${titleFont}"; font-size: 12pt; font-weight: 600; }
        QTextBrowser#whatsNew { background: transparent; border: none; color: ${text}; font-size: 10pt; }
        #navFoot { color: ${hint}; font-size: 8.5pt; padding: 0; }
        #navParent { font-family: "${titleFont}"; font-size: 12pt; font-weight: 700; padding: 0; }
        #pageTitle { font-family: "${titleFont}"; font-size: 19pt; font-weight: 600; }
        #card { background: ${card}; border: 1px solid ${border}; border-radius: 8px; }
        #sep { background: ${sep}; }
        #hint, #footer { color: ${hint}; font-size: 9pt; }
        #warn { color: ${warn}; }
        #keycap { background: ${ctrl}; border: 1px solid ${ctrlBorder}; border-bottom: 2px solid ${ctrlBorder};
                  border-radius: 4px; padding: 2px 8px; font-weight: 600; }
        #chipSk { background: ${ctrl}; border: 1px dashed ${ctrlBorder}; border-radius: 4px; padding: 3px 6px; }
        #info { color: ${hint}; font-size: 11pt; padding: 0; }
        #info:hover { color: ${text}; }
        #chip { background: ${ctrl}; border: 1px solid ${ctrlBorder}; border-radius: 4px; padding: 3px 6px; }
        QComboBox { background: ${ctrl}; border: 1px solid ${ctrlBorder}; border-radius: 4px;
                    padding: 4px 10px; min-width: 180px; }
        QComboBox:hover { background: ${ctrlHover}; }
        QComboBox::drop-down { border: none; width: 28px; }
        QComboBox::down-arrow { image: url(${arrow}); width: 12px; height: 12px; }
        QComboBox QAbstractItemView { background: ${card}; color: ${text}; border: 1px solid ${ctrlBorder};
                                      outline: none; selection-background-color: ${navSel};
                                      selection-color: ${text}; padding: 4px; }
        QPushButton { background: ${ctrl}; border: 1px solid ${ctrlBorder}; border-radius: 4px; padding: 5px 16px; }
        QPushButton:hover { background: ${ctrlHover}; }
        QPushButton#accent { background: ${accent}; color: ${accentText}; border: 1px solid ${accent}; }
        QScrollBar:vertical { background: transparent; width: 8px; }
        QScrollBar::handle:vertical { background: ${ctrlBorder}; border-radius: 4px; min-height: 30px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
        QToolTip { background: ${card}; color: ${text}; border: 1px solid ${ctrlBorder}; padding: 6px 8px; }
        QLineEdit, QPlainTextEdit { background: ${ctrl}; color: ${text}; border: 1px solid ${ctrlBorder};
                                    border-bottom: 2px solid ${ctrlBorder}; border-radius: 4px; padding: 5px 8px;
                                    selection-background-color: ${accent}; selection-color: ${accentText}; }
        QLineEdit:focus, QPlainTextEdit:focus { border-bottom: 2px solid ${accent}; }
        QPushButton:disabled { color: ${hint}; }
        QProgressBar { background: ${ctrl}; border: none; border-radius: 2px; height: 4px; max-height: 4px; }
        QProgressBar::chunk { background: ${accent}; border-radius: 2px; }
    )");
    QMap<QString, QString> v = {
        {"font", t.font}, {"titleFont", t.titleFont}, {"bg", t.bg.name()}, {"nav", t.nav.name()},
        {"card", t.card.name()}, {"border", t.border.name()}, {"sep", t.sep.name()}, {"text", t.text.name()},
        {"hint", t.hint.name()}, {"accent", t.accent.name()}, {"accentText", t.accentText.name()},
        {"ctrl", t.ctrl.name()}, {"ctrlHover", t.ctrlHover.name()}, {"ctrlBorder", t.ctrlBorder.name()},
        {"navHover", t.navHover.name()}, {"navSel", t.navSel.name()}, {"warn", t.warn.name()},
        {"arrow", arrowImage(t.text)}, {"slogan", sloganCss(t)},
    };
    for (auto it = v.constBegin(); it != v.constEnd(); ++it)
        s.replace("${" + it.key() + "}", it.value());
    return s;
}

// ---------------- Cập nhật: so sánh phiên bản ----------------
static bool newerThan(const QString& a, const QString& b) {  // a > b ?
    QStringList x = a.split('.'), y = b.split('.');
    for (int i = 0; i < qMax(x.size(), y.size()); ++i) {
        int p = i < x.size() ? x[i].toInt() : 0, q = i < y.size() ? y[i].toInt() : 0;
        if (p != q) return p > q;
    }
    return false;
}

struct UpdateInfo {
    bool ok = false;
    // notice = true: không phải lỗi, chỉ là máy chủ chưa có thông tin bản mới (website chưa đăng tệp).
    // Trang Cập nhật hiện câu thông tin, không ghi "Không kiểm tra được". (0.3.19)
    bool notice = false;
    QString error, version, url, sha256, notes;
};

// Đọc tệp latest.json trên trang web (chạy trên luồng phụ).
// Từ 0.3.19 tách bốn trường hợp để người dùng hiểu đúng (trước đây mọi lỗi đều thành "không đúng định dạng"):
//   1. Không kết nối được (mạng, DNS, TLS, hết giờ)            -> lỗi "Không kết nối được máy chủ cập nhật…"
//   2. Máy chủ chưa có thông tin: HTTP 404/410, nội dung không phải JSON (trang HTML),
//      hoặc JSON không có mục DK_UPDATE_APP_KEY                 -> thông tin "Chưa có thông tin bản mới…"
//   3. Máy chủ lỗi khác (5xx, 403…)                             -> lỗi "Máy chủ cập nhật đang trục trặc…"
//   4. Có mục "dhammakey" nhưng thiếu/sai trường                -> lỗi "Thông tin cập nhật không đúng định dạng."
static UpdateInfo fetchUpdateInfo() {
    UpdateInfo u;
    net::Response r = net::get(QStringLiteral(DK_UPDATE_URL));
    const QString notYet = QStringLiteral("Chưa có thông tin bản mới trên máy chủ. DhammaKey sẽ thử lại sau.");
    if (r.status == 0 || (!r.error.isEmpty() && r.status >= 200 && r.status < 300)) {  // không nối được hoặc đứt giữa chừng
        u.error = r.error == QStringLiteral("Đã hủy.") ? r.error
                : QStringLiteral("Không kết nối được máy chủ cập nhật. Hãy kiểm tra kết nối Internet rồi thử lại.");
        return u;
    }
    if (r.status == 404 || r.status == 410) { u.notice = true; u.error = notYet; return u; }
    if (!r.ok()) {
        u.error = QStringLiteral("Máy chủ cập nhật đang trục trặc (mã %1). Hãy thử lại sau.").arg(r.status);
        return u;
    }
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(r.body, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) { u.notice = true; u.error = notYet; return u; }
    QJsonObject root = doc.object();
    // Tệp chung của DhammaWare: mỗi ứng dụng một mục theo tên khoá ("dhammakey").
    // Vẫn nhận dạng phẳng cũ (các trường nằm ngay ở gốc).
    QJsonObject o;
    if (root.value(QStringLiteral(DK_UPDATE_APP_KEY)).isObject()) o = root.value(QStringLiteral(DK_UPDATE_APP_KEY)).toObject();
    else if (root.contains(QStringLiteral("version"))) o = root;
    else { u.notice = true; u.error = notYet; return u; }
    u.version = o.value("version").toString().trimmed();
    u.url = o.value("url").toString().trimmed();
    u.sha256 = o.value("sha256").toString().trimmed().toLower();
    u.notes = o.value("notes").toString();
    // Bắt buộc đủ ba trường; bộ cài phải tải qua HTTPS và có mã SHA-256 (64 ký tự hex) để so trước khi chạy
    static const QRegularExpression hex64(QStringLiteral("^[0-9a-f]{64}$"));
    static const QRegularExpression ver(QStringLiteral("^\\d+(\\.\\d+){1,3}$"));
    if (!ver.match(u.version).hasMatch() || !u.url.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive) ||
        !hex64.match(u.sha256).hasMatch()) {
        u.error = QStringLiteral("Thông tin cập nhật trên máy chủ không đúng định dạng.");
        return u;
    }
    u.ok = true;
    return u;
}

// ---------------- Công tắc gạt kiểu Windows 11 ----------------
class Toggle : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal pos READ pos WRITE setPos)
public:
    explicit Toggle(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        anim_ = new QPropertyAnimation(this, "pos", this);
        anim_->setDuration(120);
        connect(this, &QAbstractButton::toggled, this, [this](bool on) {
            anim_->stop();
            anim_->setStartValue(pos_);
            anim_->setEndValue(on ? 1.0 : 0.0);
            anim_->start();
        });
    }
    QSize sizeHint() const override { return QSize(40, 20); }
    qreal pos() const { return pos_; }
    void setPos(qreal p) { pos_ = p; update(); }
    void setCheckedNoAnim(bool on) {
        QSignalBlocker b(this);
        setChecked(on);
        pos_ = on ? 1.0 : 0.0;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(0.5, 0.5, width() - 1, height() - 1);
        qreal rad = r.height() / 2;
        bool hover = underMouse();
        if (isChecked()) {
            p.setPen(Qt::NoPen);
            p.setBrush(hover ? g_theme.accent.lighter(112) : g_theme.accent);
        } else {
            p.setPen(QPen(g_theme.hint, 1));
            p.setBrush(hover ? g_theme.ctrlHover : g_theme.ctrl);
        }
        p.drawRoundedRect(r, rad, rad);
        qreal d = isChecked() ? 12 : 10;
        qreal x = 5 + pos_ * (width() - 20);
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? g_theme.accentText : g_theme.hint);
        p.drawEllipse(QPointF(x + 5, height() / 2.0), d / 2, d / 2);
    }
    void enterEvent(QEvent*) override { update(); }
    void leaveEvent(QEvent*) override { update(); }

private:
    qreal pos_ = 0;
    QPropertyAnimation* anim_;
};

// ---------------- Ô chọn skin (có hình xem trước) ----------------
class SkinTile : public QAbstractButton {
public:
    SkinTile(int skin, QWidget* parent = nullptr) : QAbstractButton(parent), t_(makeTheme(skin)) {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(170, 150);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF outer(1, 1, width() - 2, 112);
        // Khung xem trước
        p.setPen(QPen(isChecked() ? g_theme.accent : g_theme.ctrlBorder, isChecked() ? 2.5 : 1));
        p.setBrush(t_.bg);
        p.drawRoundedRect(outer, 8, 8);
        QRectF nav(outer.left() + 8, outer.top() + 10, 36, outer.height() - 20);
        p.setPen(Qt::NoPen);
        p.setBrush(t_.nav);
        p.drawRoundedRect(nav, 4, 4);
        p.setBrush(t_.accent);
        p.drawRoundedRect(QRectF(nav.left() + 4, nav.top() + 8, 3, 10), 1.5, 1.5);
        QRectF card(nav.right() + 8, outer.top() + 10, outer.right() - nav.right() - 16, outer.height() - 20);
        p.setPen(QPen(t_.border, 1));
        p.setBrush(t_.card);
        p.drawRoundedRect(card, 5, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(t_.text);
        p.drawRoundedRect(QRectF(card.left() + 8, card.top() + 10, card.width() * 0.55, 6), 3, 3);
        p.setBrush(t_.hint);
        p.drawRoundedRect(QRectF(card.left() + 8, card.top() + 22, card.width() * 0.4, 4), 2, 2);
        p.setBrush(t_.accent);
        p.drawRoundedRect(QRectF(card.right() - 30, card.top() + 10, 22, 11), 5.5, 5.5);
        p.setBrush(t_.text);
        p.drawRoundedRect(QRectF(card.left() + 8, card.top() + 44, card.width() * 0.6, 6), 3, 3);
        p.setBrush(t_.hint);
        p.drawRoundedRect(QRectF(card.left() + 8, card.top() + 56, card.width() * 0.35, 4), 2, 2);
        // Tên skin
        QFont f(g_theme.font);
        f.setPointSizeF(10);
        f.setBold(isChecked());
        p.setFont(f);
        p.setPen(g_theme.text);
        p.drawText(QRectF(0, 118, width(), 28), Qt::AlignCenter, t_.name);
    }

private:
    Theme t_;
};

// ---------------- Biểu tượng nét mảnh (vẽ bằng mã, đổi màu theo skin) ----------------
// Hệ tọa độ 24×24, nét 1.6 — cùng tinh thần biểu tượng của Windows 11, nhưng chạy được cả Win7.
static void paintIcon(QPainter& p, const QString& name, const QColor& c) {
    QPen pen(c, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    auto dot = [&](qreal x, qreal y, qreal r = 1.0) {
        p.save(); p.setPen(Qt::NoPen); p.setBrush(c); p.drawEllipse(QPointF(x, y), r, r); p.restore();
    };
    if (name.startsWith("glyph:")) {
        QFont f(g_theme.font);
        f.setPixelSize(17);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.drawText(QRectF(0, -1, 24, 24), Qt::AlignCenter, name.mid(6));
    } else if (name == "keyboard") {
        p.drawRoundedRect(QRectF(2.5, 6, 19, 12), 2, 2);
        for (qreal x : {6.0, 9.0, 12.0, 15.0, 18.0}) dot(x, 9.6, 0.9);
        for (qreal x : {7.5, 10.5, 13.5, 16.5}) dot(x, 12.4, 0.9);
        p.drawLine(QPointF(8, 15.2), QPointF(16, 15.2));
    } else if (name == "keys") {
        p.drawRoundedRect(QRectF(4, 4, 16, 16), 3, 3);
        p.drawRoundedRect(QRectF(7, 7, 10, 8), 1.5, 1.5);
    } else if (name == "check") {
        p.drawPolyline(QPolygonF({QPointF(4.5, 12.5), QPointF(9.5, 17.5), QPointF(19.5, 6.5)}));
    } else if (name == "power") {
        p.drawArc(QRectF(5, 5.5, 14, 14), 60 * 16, -300 * 16);
        p.drawLine(QPointF(12, 3.5), QPointF(12, 11));
    } else if (name == "mouse") {
        p.drawPolygon(QPolygonF({QPointF(6, 3), QPointF(6, 19), QPointF(10, 15.3), QPointF(12.8, 21),
                                 QPointF(15.3, 19.8), QPointF(12.6, 14.2), QPointF(18, 14)}));
    } else if (name == "shield") {
        QPainterPath s;
        s.moveTo(12, 3); s.lineTo(19, 6); s.lineTo(19, 11);
        s.cubicTo(19, 16, 16, 19.5, 12, 21); s.cubicTo(8, 19.5, 5, 16, 5, 11);
        s.lineTo(5, 6); s.closeSubpath();
        p.drawPath(s);
        p.drawLine(QPointF(12, 8), QPointF(12, 13));
        dot(12, 16.3);
    } else if (name == "bell") {
        QPainterPath b;
        b.moveTo(6, 16); b.lineTo(6, 11); b.arcTo(QRectF(6, 5, 12, 12), 180, -180);
        b.lineTo(18, 16); b.lineTo(19.5, 18); b.lineTo(4.5, 18); b.lineTo(6, 16);
        p.drawPath(b);
        p.drawLine(QPointF(10.5, 20.8), QPointF(13.5, 20.8));
        p.drawLine(QPointF(12, 3.2), QPointF(12, 5));
    } else if (name == "palette") {
        p.drawEllipse(QRectF(3, 3, 18, 18));
        dot(8.2, 9.5, 1.3); dot(12, 7.2, 1.3); dot(15.8, 9.5, 1.3); dot(8.6, 14, 1.3);
        p.drawEllipse(QRectF(13, 13, 4.5, 4.5));
    } else if (name == "refresh") {
        p.drawArc(QRectF(4, 4, 16, 16), 40 * 16, 290 * 16);
        QPointF tip(12 + 8 * 0.766, 12 - 8 * 0.643);
        p.drawLine(tip, tip + QPointF(0.4, -4.4));
        p.drawLine(tip, tip + QPointF(-4.3, -0.6));
    } else if (name == "calendar") {
        p.drawRoundedRect(QRectF(3.5, 5, 17, 15), 2, 2);
        p.drawLine(QPointF(3.5, 9.5), QPointF(20.5, 9.5));
        p.drawLine(QPointF(8, 3), QPointF(8, 6.5));
        p.drawLine(QPointF(16, 3), QPointF(16, 6.5));
        dot(12, 14.6, 1.2);
    } else if (name == "tag") {
        p.drawPolygon(QPolygonF({QPointF(3.5, 4.5), QPointF(11, 4.5), QPointF(20, 13.5), QPointF(13.5, 20),
                                 QPointF(3.5, 11)}));
        dot(8, 8.6, 1.2);
    } else if (name == "monitor") {
        p.drawRoundedRect(QRectF(3, 4, 18, 12), 1.5, 1.5);
        p.drawLine(QPointF(12, 16), QPointF(12, 20));
        p.drawLine(QPointF(8.5, 20), QPointF(15.5, 20));
    } else if (name == "chat") {
        QPainterPath b;
        b.addRoundedRect(QRectF(3, 4, 18, 13), 3, 3);
        p.drawPath(b);
        p.drawPolyline(QPolygonF({QPointF(8, 17), QPointF(7, 21), QPointF(12, 17)}));
        dot(8.5, 10.5); dot(12, 10.5); dot(15.5, 10.5);
    }
}

static QPixmap iconPixmap(const QString& name, const QColor& c, int size = 20) {
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.scale(size / 24.0, size / 24.0);
    paintIcon(p, name, c);
    return pm;
}

// Các nhãn biểu tượng đang hiện, để vẽ lại khi đổi skin
static QVector<QPair<QPointer<QLabel>, QString>> g_iconLabels;

static QLabel* iconLabel(const QString& name) {
    QLabel* l = new QLabel;
    l->setFixedSize(22, 22);
    l->setAlignment(Qt::AlignCenter);
    l->setPixmap(iconPixmap(name, g_theme.text));
    g_iconLabels.append({QPointer<QLabel>(l), name});
    return l;
}

// Biểu tượng cho từng dòng cài đặt, theo tiêu đề dòng
static QString iconForRow(const QString& title) {
    static const QVector<QPair<QString, QString>> map = {
        {QStringLiteral("Kiểu gõ tiếng Việt"), "keyboard"},
        {QStringLiteral("Bỏ dấu kiểu mới"), QStringLiteral("glyph:à")},
        {QStringLiteral("Kiểm tra chính tả"), "check"},
        {QStringLiteral("Telex: w"), QStringLiteral("glyph:ư")},
        {QStringLiteral("Bảng mã"), QStringLiteral("glyph:ẫ")},
        {QStringLiteral("Chữ niggahīta"), QStringLiteral("glyph:ṃ")},
        {QStringLiteral("Chế độ Pāli"), QStringLiteral("glyph:ā")},
        {QStringLiteral("Gõ nhanh bằng AltGr"), "keys"},
        {QStringLiteral("Ô mật khẩu"), "shield"},
        {QStringLiteral("Thanh địa chỉ"), "mouse"},
        {QStringLiteral("Cửa sổ dòng lệnh"), "monitor"},
        {QStringLiteral("Ô email"), "keyboard"},
        {QStringLiteral("Bật / tắt tiếng Việt"), "power"},
        {QStringLiteral("Bật DhammaKey khi mở máy"), "power"},
        {QStringLiteral("Bật / tắt gõ Pāli"), QStringLiteral("glyph:ñ")},
        {QStringLiteral("Chọn kiểu gõ"), "mouse"},
        {QStringLiteral("Nhắc khi"), "bell"},
        {QStringLiteral("Giao diện"), "palette"},
        {QStringLiteral("Chữ trên biểu tượng"), "keyboard"},
        {QStringLiteral("Đang dùng DhammaKey"), "refresh"},
        {QStringLiteral("Tự kiểm tra cập nhật"), "calendar"},
        {QStringLiteral("Loại"), "tag"},
        {QStringLiteral("Gửi kèm phiên bản"), "monitor"},
    };
    if (title.isEmpty()) return QStringLiteral("shield");   // dòng trạng thái "Bộ gõ khác"
    for (const auto& m : map)
        if (title.startsWith(m.first)) return m.second;
    return QString();
}

// ---------------- Khung giao diện ----------------
static QVBoxLayout* makeCard(QVBoxLayout* page) {
    QFrame* card = new QFrame;
    card->setObjectName("card");
    QVBoxLayout* body = new QVBoxLayout(card);
    body->setContentsMargins(16, 4, 16, 4);
    body->setSpacing(0);
    page->addWidget(card);
    page->addSpacing(12);
    return body;
}

static void addSeparator(QVBoxLayout* body) {
    if (body->count() == 0) return;
    QFrame* line = new QFrame;
    line->setObjectName("sep");
    line->setFixedHeight(1);
    body->addWidget(line);
}

// Một dòng: tiêu đề + chú thích bên trái, điều khiển bên phải
// ⓘ: rê chuột vào hiện hướng dẫn (0.3.31). Văn bản có thể là HTML đơn giản.
static QLabel* infoIcon(const QString& tip) {
    QLabel* i = new QLabel(QStringLiteral("ⓘ"));
    i->setObjectName("info");
    i->setCursor(Qt::WhatsThisCursor);
    i->setToolTip(QStringLiteral("<div style='white-space:normal'>") + tip + QStringLiteral("</div>"));
    i->setToolTipDuration(30000);
    return i;
}

static QWidget* addRow(QVBoxLayout* body, const QString& title, const QString& hint, QWidget* control,
                       QLabel** titleOut = nullptr, const QString& help = QString(), QLabel** infoOut = nullptr) {
    addSeparator(body);
    QWidget* row = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 10, 0, 10);
    h->setSpacing(16);
    QString icon = iconForRow(title);
    if (!icon.isEmpty()) h->addWidget(iconLabel(icon), 0, Qt::AlignVCenter);
    QVBoxLayout* text = new QVBoxLayout;
    text->setSpacing(2);
    QLabel* t = new QLabel(title);
    t->setWordWrap(true);
    if (!help.isEmpty()) {
        QHBoxLayout* th = new QHBoxLayout;
        th->setSpacing(6);
        th->setContentsMargins(0, 0, 0, 0);
        th->addWidget(t, 0);
        QLabel* inf = infoIcon(help);
        if (infoOut) *infoOut = inf;
        th->addWidget(inf, 0, Qt::AlignVCenter);
        th->addStretch(1);
        text->addLayout(th);
    } else {
        text->addWidget(t);
    }
    if (titleOut) *titleOut = t;
    if (!hint.isEmpty()) {
        QLabel* s = new QLabel(hint);
        s->setObjectName("hint");
        s->setWordWrap(true);
        text->addWidget(s);
    }
    h->addLayout(text, 1);
    if (control) h->addWidget(control, 0, Qt::AlignVCenter);
    body->addWidget(row);
    return row;
}

static Toggle* addToggle(QVBoxLayout* body, const QString& title, const QString& hint,
                         const wchar_t* reg, int def, bool inverted = false, const QString& help = QString()) {
    Toggle* t = new Toggle;
    bool on = store::get(reg, def) != 0;
    t->setCheckedNoAnim(inverted ? !on : on);
    QObject::connect(t, &QAbstractButton::toggled, [reg, inverted](bool checked) {
        store::set(reg, (inverted ? !checked : checked) ? 1 : 0);
    });
    addRow(body, title, hint, t, nullptr, help);
    return t;
}

// Hướng dẫn ⓘ của trang Kiểu gõ, theo kiểu đang chọn
static QString methodHelp(int m) {
    if (m == DK_METHOD_VNI)
        return QStringLiteral("<b>VNI</b> — dấu bằng số:<br>1 sắc · 2 huyền · 3 hỏi · 4 ngã · 5 nặng<br>"
                              "6 â ê ô · 7 ơ ư · 8 ă · 9 đ<br>Ví dụ: <b>a6</b> → â, <b>o7</b> → ơ, <b>d9</b> → đ, "
                              "<b>Viet65</b> → Việt.");
    if (m == DK_METHOD_VIQR)
        return QStringLiteral("<b>VIQR</b> — dấu bằng ký hiệu:<br>' sắc · ` huyền · ? hỏi · ~ ngã · . nặng<br>"
                              "^ â ê ô · + ơ ư · ( ă · dd đ<br>Ví dụ: <b>a^</b> → â, <b>o+</b> → ơ, <b>dd</b> → đ, "
                              "<b>Vie^.t</b> → Việt.");
    return QStringLiteral("<b>Telex</b> — dấu bằng chữ:<br>s sắc · f huyền · r hỏi · x ngã · j nặng · z xoá dấu<br>"
                          "aa â · ee ê · oo ô · aw ă · ow ơ · uw ư · dd đ<br>Ví dụ: <b>aa</b> → â, <b>dd</b> → đ, "
                          "<b>Vieetj</b> → Việt.");
}

static QLabel* keycap(const QString& text) {
    QLabel* k = new QLabel(text);
    k->setObjectName("keycap");
    return k;
}

// Trang nội dung: tiêu đề lớn + các thẻ; bọc trong vùng cuộn phòng khi màn hình nhỏ
static QVBoxLayout* makePage(QStackedWidget* stack, const QString& title) {
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget* body = new QWidget;
    body->setObjectName("scrollBody");
    QVBoxLayout* v = new QVBoxLayout(body);
    v->setContentsMargins(28, 22, 28, 18);
    v->setSpacing(0);
    QLabel* t = new QLabel(title);
    t->setObjectName("pageTitle");
    v->addWidget(t);
    v->addSpacing(16);
    scroll->setWidget(body);
    stack->addWidget(scroll);
    return v;
}

// Logo phím ṃ, thu nhỏ mịn từ ảnh 256 px; dpr = tỉ lệ màn hình (125%, 150%…) để không bị nhoè
static QPixmap logoPixmap(int size, qreal dpr) {
    QPixmap src(QStringLiteral(":/logo.png"));
    if (src.isNull()) return QPixmap();
    int px = qRound(size * dpr);
    QPixmap pm = src.scaled(px, px, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pm.setDevicePixelRatio(dpr);
    return pm;
}

// Cỡ bánh xe DhammaWare ở chân cột trái: to để thương hiệu mẹ là chủ đạo (người dùng yêu cầu, bản 0.3.8)
static const int kWheelSize = 44;
static const int kWheelPad = 6;     // lề quanh bánh xe cho quầng sáng khi loé

// Logo mẹ DhammaWare cỡ nhỏ: bánh xe Pháp rút gọn (tám nan, không chữ — dưới 96 px theo hướng dẫn thương hiệu).
// Màu cam #D97757 (người dùng chọn 14/11/2026), giống nhau ở cả ba skin.
static QPixmap wheelPixmap(int size, qreal dpr, const Theme& t) {
    Q_UNUSED(t);
    QColor c("#D97757");
    int px = qRound(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    // Hình theo svg/dhammaware-wheel-small.svg (khung 100): vành r 38 nét 9, nan từ r 9 đến r 36 nét 6.5,
    // nan lệch 22.5°, trục r 8.
    p.scale(size / 100.0, size / 100.0);
    p.translate(50, 50);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c, 9));
    p.drawEllipse(QPointF(0, 0), 38, 38);
    p.setPen(QPen(c, 6.5, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 8; ++i) {               // tám nan = Bát chánh đạo
        qreal a = (22.5 + i * 45) * M_PI / 180;
        p.drawLine(QPointF(qCos(a) * 9, qSin(a) * 9), QPointF(qCos(a) * 36, qSin(a) * 36));
    }
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(0, 0), 8, 8);         // trục
    return pm;
}

// ---------------- Bánh xe DhammaWare có hiệu ứng (dùng ở chân cột trái và cửa sổ Về chúng tôi) ----------------
struct EmbossColors { QColor hi, lo; };
static EmbossColors embossColors(const Theme& t) {
    if (t.dark) return {QColor(255, 255, 255, 46), QColor(0, 0, 0, 170)};
    return {QColor(255, 255, 255, 220), QColor(0, 0, 0, 80)};
}

// Tô một hình (giữ kênh alpha) bằng một màu
static QPixmap tinted(const QPixmap& src, const QColor& c) {
    QPixmap out(src.size());
    out.setDevicePixelRatio(src.devicePixelRatio());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.drawPixmap(0, 0, src);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), c);
    return out;
}

// Bánh xe DhammaWare nổi (emboss) ở cửa sổ "Về chúng tôi".
// Bản 0.3.15 (người dùng yêu cầu): xoay chậm liên tục (một vòng 24 giây) và thỉnh thoảng loé sáng
// (khoảng 8–12 giây một lần, mỗi lần ~0,7 giây: bánh xe sáng lên, quầng cam mờ toả ra).
// Bản 0.3.16: luôn quay (người dùng muốn quay; bản 0.3.15 đứng yên trên máy tắt "Animation effects"),
// đồng hồ và bộ đếm chạy ngay từ lúc tạo, không chờ showEvent.
// Bản 0.3.18 (làm lại thay đổi của 0.3.17): bánh xe ở chân cột trái Bảng điều khiển cũng xoay và loé sáng —
// EmbossWheel(44, false, 6): không nổi (chỉ quay + loé), lề 6 px cho quầng sáng.
class EmbossWheel : public QWidget {
public:
    explicit EmbossWheel(int size, bool emboss = true, int pad = 10, QWidget* parent = nullptr)
        : QWidget(parent), size_(size), emboss_(emboss), pad_(pad) {
        setFixedSize(size + 2 * pad_, size + 2 * pad_);
        animate_ = true;
        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        timer_->setInterval(33);                           // ~30 hình/giây là đủ mượt cho vòng quay chậm
        connect(timer_, &QTimer::timeout, this, [this] { update(); });
        nextFlash_ = 2500;                                 // lần loé đầu: sau khi mở ~2,5 giây
        clock_.start();
        timer_->start();
    }
    // Đổi skin: vẽ lại hình (màu bóng nổi theo skin)
    void refresh() { base_ = QPixmap(); update(); }
protected:
    void showEvent(QShowEvent* e) override {
        if (timer_ && !timer_->isActive()) timer_->start();
        QWidget::showEvent(e);
    }
    void hideEvent(QHideEvent* e) override {
        if (timer_) timer_->stop();
        QWidget::hideEvent(e);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        ensurePixmaps();
        const qint64 ms = clock_.isValid() ? clock_.elapsed() : 0;
        const qreal angle = animate_ ? std::fmod(ms * 360.0 / kTurnMs, 360.0) : 0.0;

        // Độ loé 0..1: lên rồi xuống theo nửa hình sin
        qreal f = 0;
        if (animate_) {
            if (ms >= nextFlash_ + kFlashMs) {             // hết lần loé này → hẹn lần sau, lệch ngẫu nhiên cho tự nhiên
                nextFlash_ = ms + 8000 + QRandomGenerator::global()->bounded(4000);
            } else if (ms >= nextFlash_) {
                f = qSin(M_PI * (ms - nextFlash_) / kFlashMs);
            }
        }
        const QPointF c(width() / 2.0, height() / 2.0);
        if (f > 0) {                                       // quầng cam phía sau bánh xe
            QRadialGradient g(c, size_ / 2.0 + pad_);
            g.setColorAt(0.0, QColor(217, 119, 87, int(150 * f)));
            g.setColorAt(0.55, QColor(217, 119, 87, int(90 * f)));
            g.setColorAt(1.0, QColor(217, 119, 87, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawEllipse(c, size_ / 2.0 + pad_, size_ / 2.0 + pad_);
        }
        // Bóng và chỗ sáng đứng yên (ánh sáng từ trên-trái), chỉ bánh xe quay
        if (emboss_) {
            const qreal d = qMax<qreal>(1.5, size_ / 40.0);    // độ nổi
            drawTurned(p, lo_, c + QPointF(d, d), angle);
            drawTurned(p, hi_, c - QPointF(d, d), angle);
        }
        drawTurned(p, base_, c, angle);
        if (f > 0) {                                       // ánh sáng phủ lên bánh xe
            p.setOpacity(0.55 * f);
            drawTurned(p, glow_, c, angle);
            p.setOpacity(1.0);
        }
    }
private:
    static const int kTurnMs = 24000;    // một vòng 24 giây
    static const int kFlashMs = 700;     // một lần loé

    void ensurePixmaps() {
        const qreal dpr = devicePixelRatioF();
        if (!base_.isNull() && qFuzzyCompare(dpr, dpr_)) return;
        dpr_ = dpr;
        base_ = wheelPixmap(size_, dpr, g_theme);
        EmbossColors ec = embossColors(g_theme);
        hi_ = tinted(base_, ec.hi);
        lo_ = tinted(base_, ec.lo);
        glow_ = tinted(base_, QColor(255, 236, 224));
    }
    void drawTurned(QPainter& p, const QPixmap& pm, const QPointF& center, qreal angle) {
        p.save();
        p.translate(center);
        p.rotate(angle);
        p.drawPixmap(QPointF(-size_ / 2.0, -size_ / 2.0), pm);
        p.restore();
    }
    int size_;
    bool emboss_ = true;
    int pad_ = 10;                       // chỗ cho bóng và quầng sáng
    bool animate_ = false;
    QTimer* timer_ = nullptr;
    QElapsedTimer clock_;
    qint64 nextFlash_ = 0;
    qreal dpr_ = 0;
    QPixmap base_, hi_, lo_, glow_;
};

#ifdef _WIN32
// ---------------- Thanh tiêu đề tự vẽ (Windows) ----------------
// Thanh tiêu đề của Windows chỉ cho biểu tượng 16 px và chữ nhỏ, nên Bảng điều khiển tự vẽ thanh tiêu đề:
// logo 28 px, chữ to và đậm, hai nút thu nhỏ / đóng kiểu Windows 11.
// Cửa sổ cỡ cố định vừa đủ các trang: không phóng to, không kéo giãn (người dùng chốt 14/11/2026).
// Khung vẫn là của Windows: bóng đổ, kéo thanh tiêu đề để dời, chuột phải ra menu hệ thống.
static const int kTitleH = 46;     // chiều cao thanh tiêu đề
static const int kCapBtnW = 46;    // bề ngang mỗi nút
static const int kNavW = 210;      // bề ngang cột trái (để tô hai màu cho liền với hai cột bên dưới)

class CaptionButton : public QAbstractButton {
public:
    enum Kind { Min, Max, Close };
    CaptionButton(Kind k, QWidget* parent) : QAbstractButton(parent), k_(k) {
        setFixedSize(kCapBtnW, kTitleH);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover);
    }
    void setMaximized(bool m) { maxed_ = m; update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        bool hot = underMouse(), down = isDown();
        QColor fg = window()->isActiveWindow() ? g_theme.text : g_theme.hint;
        if (k_ == Close && (hot || down)) {
            p.fillRect(rect(), down ? QColor("#a8271b") : QColor("#c42b1c"));
            fg = Qt::white;
        } else if (hot || down) {
            p.fillRect(rect(), down ? g_theme.navSel : g_theme.navHover);
        }
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(fg, 1.0));
        p.translate(width() / 2.0, height() / 2.0);
        const qreal r = 5;  // nửa cạnh hình vẽ (10 px)
        if (k_ == Min) {
            p.drawLine(QPointF(-r, 0.5), QPointF(r, 0.5));
        } else if (k_ == Max) {
            if (!maxed_) {
                p.drawRoundedRect(QRectF(-r + 0.5, -r + 0.5, 2 * r - 1, 2 * r - 1), 1.5, 1.5);
            } else {  // hai ô chồng nhau = Thu về
                p.drawRoundedRect(QRectF(-r + 0.5, -r + 2.5, 2 * r - 3, 2 * r - 3), 1.5, 1.5);
                QPainterPath back;
                back.moveTo(-r + 2.5, -r + 0.5);
                back.lineTo(r - 1.5, -r + 0.5);
                back.quadTo(r - 0.5, -r + 0.5, r - 0.5, -r + 1.5);
                back.lineTo(r - 0.5, r - 2.5);
                p.drawPath(back);
            }
        } else {
            p.drawLine(QPointF(-r, -r), QPointF(r, r));
            p.drawLine(QPointF(-r, r), QPointF(r, -r));
        }
    }
    void enterEvent(QEvent* e) override { QAbstractButton::enterEvent(e); update(); }
    void leaveEvent(QEvent* e) override { QAbstractButton::leaveEvent(e); update(); }
private:
    Kind k_;
    bool maxed_ = false;
};

class TitleBar : public QWidget {
public:
    explicit TitleBar(QWidget* win) : QWidget(win) {
        setFixedHeight(kTitleH);
        QHBoxLayout* h = new QHBoxLayout(this);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(0);
        h->addStretch(1);
        minBtn = new CaptionButton(CaptionButton::Min, this);
        closeBtn = new CaptionButton(CaptionButton::Close, this);
        minBtn->setToolTip(QStringLiteral("Thu nhỏ"));
        closeBtn->setToolTip(QStringLiteral("Đóng"));
        h->addWidget(minBtn);
        h->addWidget(closeBtn);
        connect(minBtn, &QAbstractButton::clicked, win, &QWidget::showMinimized);
        connect(closeBtn, &QAbstractButton::clicked, win, &QWidget::close);
    }
    void syncState() {
        update();
        minBtn->update();
        closeBtn->update();
    }
    CaptionButton *minBtn, *closeBtn;
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        // Tô hai màu cho liền với cột trái và vùng nội dung bên dưới
        p.fillRect(QRect(0, 0, kNavW, height()), g_theme.nav);
        p.fillRect(QRect(kNavW, 0, width() - kNavW, height()), g_theme.bg);
        // Logo 28 px
        const int icon = 28, x0 = 14;
        p.drawPixmap(x0, (height() - icon) / 2, logoPixmap(icon, devicePixelRatioF()));
        // Tiêu đề: to, đậm; mờ đi khi cửa sổ không được chọn (giống Windows)
        QFont f(g_theme.titleFont);
        f.setPointSizeF(13);
        f.setWeight(QFont::Bold);
        p.setFont(f);
        p.setPen(window()->isActiveWindow() ? g_theme.text : g_theme.hint);
        int tx = x0 + icon + 10;
        const int avail = width() - tx - 2 * kCapBtnW - 8;
        QFontMetrics fm(f);
        const QString title = fm.elidedText(window()->windowTitle(), Qt::ElideRight, avail);
        const int base = (height() - fm.height()) / 2 + fm.ascent();   // đường chân chữ của tiêu đề
        p.drawText(tx, base, title);
        // Slogan đi kèm logo (bộ nhận diện 1.4): chữ nhỏ cạnh tiêu đề, cùng đường chân chữ;
        // mờ đi khi cửa sổ không được chọn; không đủ chỗ thì bỏ.
        QFont sf(g_theme.font);
        sf.setPointSizeF(9);
        QFontMetrics sfm(sf);
        const int sx = tx + fm.horizontalAdvance(title) + 14;
        const QString slogan = QStringLiteral(DK_SLOGAN);
        if (sx + sfm.horizontalAdvance(slogan) <= tx + avail) {
            p.setFont(sf);
            QColor sc = sloganColor(g_theme);
            if (!window()->isActiveWindow()) sc.setAlphaF(0.55);
            p.setPen(sc);
            p.drawText(sx, base, slogan);
        }
    }
};
#endif

// ---------------- Cửa sổ chính ----------------
class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    SettingsWindow() {
        setObjectName("root");
        setWindowTitle(QStringLiteral("Bảng điều khiển DhammaKey"));

        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);
#ifdef _WIN32
        setupCustomFrame();
        titleBar_ = new TitleBar(this);
        outer->addWidget(titleBar_);
#endif
        QHBoxLayout* main = new QHBoxLayout;
        main->setContentsMargins(0, 0, 0, 0);
        main->setSpacing(0);
        outer->addLayout(main, 1);

        // --- Cột trái: danh sách mục ---
        QWidget* side = new QWidget;
        side->setObjectName("nav");
        side->setFixedWidth(210);  // = kNavW
        QVBoxLayout* sv = new QVBoxLayout(side);
        sv->setContentsMargins(10, 16, 10, 12);
#ifndef _WIN32
        // Đầu cột trái: logo phím ṃ + chữ DhammaKey (Windows đã có ở thanh tiêu đề tự vẽ nên bỏ, tránh lặp)
        QHBoxLayout* head = new QHBoxLayout;
        head->setContentsMargins(4, 0, 0, 0);
        head->setSpacing(10);
        logo_ = new QLabel;
        logo_->setObjectName("navLogo");
        logo_->setFixedSize(32, 32);
        logo_->setPixmap(logoPixmap(32, devicePixelRatioF()));
        head->addWidget(logo_, 0, Qt::AlignVCenter);
        QVBoxLayout* brandCol = new QVBoxLayout;
        brandCol->setSpacing(0);
        QLabel* brand = new QLabel(QStringLiteral("DhammaKey"));
        brand->setObjectName("navTitle");
        brandCol->addWidget(brand);
        QLabel* slogan = new QLabel(QStringLiteral(DK_SLOGAN));  // slogan đi kèm logo, dưới chữ, canh trái
        slogan->setObjectName("navSlogan");
        brandCol->addWidget(slogan);
        navBrand_ = brand;
        navSlogan_ = slogan;
        head->addLayout(brandCol, 1);
        sv->addLayout(head);
        sv->addSpacing(10);
#else
        sv->setContentsMargins(10, 4, 10, 12);
#endif
        nav_ = new QListWidget;
        nav_->setObjectName("nav");
        nav_->setFrameShape(QFrame::NoFrame);
        nav_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        nav_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sv->addWidget(nav_, 1);
        // Chân cột trái: logo mẹ DhammaWare (bánh xe Pháp rút gọn) + phiên bản
        QFrame* footSep = new QFrame;
        footSep->setObjectName("sep");
        footSep->setFixedHeight(1);
        sv->addWidget(footSep);
        sv->addSpacing(qMax(0, 8 + 2 - kWheelPad));
        QHBoxLayout* foot = new QHBoxLayout;
        // Bánh xe xoay và loé sáng như ở cửa sổ Về chúng tôi (0.3.17, làm lại ở 0.3.18); lề 6 px cho quầng sáng
        // nên bỏ lề trái và rút khoảng cách để chữ giữ nguyên chỗ cũ (bánh xe lệch phải 2 px).
        foot->setContentsMargins(0, 0, 0, 0);
        foot->setSpacing(12 + 4 - 2 * kWheelPad);
        wheel_ = new EmbossWheel(kWheelSize, false, kWheelPad);
        foot->addWidget(wheel_, 0, Qt::AlignVCenter);
        QVBoxLayout* footText = new QVBoxLayout;
        footText->setSpacing(0);
        QLabel* parent = new QLabel(QStringLiteral("DhammaWare™"));
        parent->setObjectName("navParent");
        QLabel* ver = new QLabel(QStringLiteral("Phiên bản %1 · dāna").arg(QStringLiteral(DK_VERSION)));
        ver->setObjectName("navFoot");
        QLabel* copy = new QLabel(QStringLiteral("© %1 %2").arg(QStringLiteral(DK_COPYRIGHT_YEAR), QStringLiteral(DK_COPYRIGHT_HOLDER)));
        copy->setObjectName("navFoot");
        footText->addWidget(parent);
        footText->addWidget(ver);
        footText->addWidget(copy);
        foot->addLayout(footText, 1);
        sv->addLayout(foot);
        main->addWidget(side);

        stack_ = new QStackedWidget;
        main->addWidget(stack_, 1);

        buildTypingPage();
        buildPaliPage();
        buildShortcutPage();
        buildGuardPage();
        buildConflictPage();
        buildSkinPage();
        buildUpdatePage();
        buildFeedbackPage();

        const bool en = uiEnglish();
        auto T = [en](const char* vi, const char* e) { return QString::fromUtf8(en ? e : vi); };
        const QPair<QString, QString> pages[] = {
            {T("Kiểu gõ", "Input method"), "keyboard"},        {T("Pāli và Sanskrit", "Pāli and Sanskrit"), QStringLiteral("glyph:ā")},
            {T("Phím tắt", "Shortcuts"), "keys"},            {T("Tự tắt gõ dấu", "Auto-off diacritics"), "power"},
            {T("Bộ gõ khác", "Other input methods"), "shield"},
            {T("Giao diện", "Appearance"), "palette"},        {T("Cập nhật", "Updates"), "refresh"},
            {T("Góp ý", "Feedback"), "chat"}};
        nav_->setIconSize(QSize(20, 20));
        for (const auto& pg : pages) {
            QListWidgetItem* it = new QListWidgetItem(pg.first, nav_);
            it->setData(Qt::UserRole, pg.second);
        }
        connect(nav_, &QListWidget::currentRowChanged, stack_, &QStackedWidget::setCurrentIndex);
        nav_->setCurrentRow(0);

#ifdef _WIN32
        setFixedSize(840, 560 + kTitleH);  // cỡ cố định vừa đủ các trang (không phóng to, không kéo giãn)
#else
        resize(840, 560);
        setMinimumSize(760, 500);
#endif
    }

    void applySkin(int skin) {
        g_theme = makeTheme(skin);
        qApp->setStyleSheet(styleSheetFor(g_theme));
        QPalette pal = qApp->palette();
        pal.setColor(QPalette::Window, g_theme.bg);
        pal.setColor(QPalette::WindowText, g_theme.text);
        pal.setColor(QPalette::Base, g_theme.card);
        pal.setColor(QPalette::Text, g_theme.text);
        pal.setColor(QPalette::Button, g_theme.ctrl);
        pal.setColor(QPalette::ButtonText, g_theme.text);
        pal.setColor(QPalette::Highlight, g_theme.accent);
        pal.setColor(QPalette::HighlightedText, g_theme.accentText);
        qApp->setPalette(pal);
#ifdef _WIN32
        // Thanh tiêu đề tối theo skin (Windows 10 1809+; máy cũ bỏ qua)
        BOOL dark = g_theme.dark ? TRUE : FALSE;
        HWND hwnd = (HWND)winId();
        if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark))))
            DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
#endif
        for (auto& il : g_iconLabels)
            if (il.first) il.first->setPixmap(iconPixmap(il.second, g_theme.text));
        for (int i = 0; i < nav_->count(); ++i)
            nav_->item(i)->setIcon(QIcon(iconPixmap(nav_->item(i)->data(Qt::UserRole).toString(), g_theme.text)));
        // Vẽ lại logo theo tỉ lệ màn hình thật (lúc dựng cửa sổ có thể chưa biết)
        if (logo_) logo_->setPixmap(logoPixmap(32, devicePixelRatioF()));
        // Mẫu A: slogan dài bằng chữ "DhammaKey" (tính lại theo phông của skin)
        if (navBrand_ && navSlogan_) {
            navBrand_->ensurePolished();
            // style sheet chung đặt cỡ chữ cho mọi QWidget, nên cỡ riêng phải đặt bằng style sheet của nhãn
            const QFont sf = sloganFontForWidth(navSlogan_->font(),
                                                QFontMetrics(navBrand_->font()).horizontalAdvance(navBrand_->text()));
            navSlogan_->setStyleSheet(QStringLiteral("font-size: %1px;").arg(sf.pixelSize()));
        }
        if (wheel_) wheel_->refresh();
#ifdef _WIN32
        if (titleBar_) titleBar_->syncState();
#endif
        for (QWidget* w : findChildren<QWidget*>()) w->update();
        refreshConflict();
    }

    enum { PAGE_UPDATE = 6, PAGE_FEEDBACK = 7 };
    void openPage(int i) { nav_->setCurrentRow(i); }
    void checkUpdateNow() { checkUpdate(); }

private:
    // --- Trang 1: Kiểu gõ + tùy chọn tiếng Việt ---
    void buildTypingPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Kiểu gõ"));
        QVBoxLayout* body = makeCard(v);
        QComboBox* method = new QComboBox;
        method->addItem(QStringLiteral("Telex"), DK_METHOD_TELEX);
        method->addItem(QStringLiteral("VNI"), DK_METHOD_VNI);
        method->addItem(QStringLiteral("VIQR"), DK_METHOD_VIQR);
        method->setCurrentIndex(qMax(0, method->findData(store::get(L"LastVietMethod", DK_METHOD_TELEX))));
        connect(method, QOverload<int>::of(&QComboBox::currentIndexChanged), [method](int) {
            int m = method->currentData().toInt();
            store::set(L"LastVietMethod", m);
            int cur = store::get(L"Method", DK_METHOD_TELEX);
            if (cur != DK_METHOD_OFF && cur != DK_METHOD_PALI_VELTHUIS) store::set(L"Method", m);
        });
        QLabel* methodInfo = nullptr;
        addRow(body, QStringLiteral("Kiểu gõ tiếng Việt"),
               QStringLiteral("Telex: aa → â, s → sắc · VNI: a6 → â, 1 → sắc · VIQR: a^ → â, ' → sắc"), method,
               nullptr, methodHelp(method->currentData().toInt()), &methodInfo);
        connect(method, QOverload<int>::of(&QComboBox::currentIndexChanged), [method, methodInfo](int) {
            if (methodInfo)
                methodInfo->setToolTip(QStringLiteral("<div style='white-space:normal'>") +
                                       methodHelp(method->currentData().toInt()) + QStringLiteral("</div>"));
        });
#ifdef _WIN32
        // Bật DhammaKey khi mở máy = đặt làm bộ gõ mặc định của Windows, và đặt lại mỗi lần đăng nhập
        // (DkLogon). Tắt: trả lại bộ gõ mặc định cũ; DhammaKey vẫn còn trong danh sách để chọn bằng Win+Space.
        Toggle* defIme = new Toggle;
        defIme->setCheckedNoAnim(store::get(L"DefaultIme", 1) != 0);
        connect(defIme, &QAbstractButton::toggled, [](bool on) {
            store::set(L"DefaultIme", on ? 1 : 0);
            QString dll = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/DhammaKey.dll");
            if (!QFileInfo::exists(dll)) return;  // chạy thử từ thư mục build: chỉ lưu lựa chọn
            wchar_t sys[MAX_PATH];
            GetSystemDirectoryW(sys, MAX_PATH);
            QProcess p;
            p.setProgram(QString::fromWCharArray(sys) + "\\rundll32.exe");
            p.setNativeArguments(QStringLiteral("\"%1\",DkSetDefault %2").arg(dll).arg(on ? 1 : 0));
            p.startDetached();
        });
        addRow(body, QStringLiteral("Bật DhammaKey khi mở máy"),
               QStringLiteral("Đặt DhammaKey làm bộ gõ mặc định của Windows. Tắt: trở về bộ gõ cũ, "
                              "vẫn chọn DhammaKey được bằng Win+Space"),
               defIme, nullptr,
               QStringLiteral("Bật: mỗi lần mở máy Windows tự dùng DhammaKey.<br>Tắt: Windows dùng bộ gõ cũ; "
                              "khi cần, bấm <b>Win + Space</b> để chọn DhammaKey."));
#endif

        body = makeCard(v);
        addToggle(body, QStringLiteral("Bỏ dấu kiểu mới"),
                  QStringLiteral("Bật: hoà, thủy · Tắt: hòa, thủy"), L"ToneNewStyle", 0, false,
                  QStringLiteral("Chọn nơi đặt dấu ở vần oa, oe, uy.<br>Bật: <b>hoà</b>, <b>thuỷ</b> (kiểu mới). "
                                 "Tắt: <b>hòa</b>, <b>thủy</b> (kiểu cũ)."));
        addToggle(body, QStringLiteral("Kiểm tra chính tả"),
                  QStringLiteral("Từ không phải tiếng Việt thì giữ nguyên phím đã gõ (buddha, majjhima)"),
                  L"SpellCheck", 1, false,
                  QStringLiteral("Bật: gõ <b>buddha</b>, <b>majjhima</b>… không bị biến thành chữ Việt.<br>"
                                 "Tắt: bộ gõ luôn cố gắng thêm dấu tiếng Việt."));
        addToggle(body, QStringLiteral("Telex: w đứng riêng thành ư"),
                  QStringLiteral("Gõ w ở đầu từ ra ư; gõ ww ra w"), L"WToU", 1, false,
                  QStringLiteral("Chỉ cho Telex. Bật: <b>w</b> ở đầu từ ra <b>ư</b>; gõ <b>ww</b> để ra chữ w.<br>"
                                 "Tắt: w chỉ dùng để thêm dấu móc (uw → ư, ow → ơ)."));
        // Bảng mã (0.3.20) — thay công tắc "Unicode tổ hợp" cũ; registry Charset (DK_CHARSET_*),
        // vẫn ghi OutputCombining cho khớp khi đang chạy DLL cũ.
        QComboBox* cs = new QComboBox;
        const struct { const char* name; int id; } charsets[] = {
            {"Unicode dựng sẵn (mặc định)", DK_CHARSET_UNICODE},
            {"Unicode tổ hợp", DK_CHARSET_UNICODE_COMBINING},
            {"TCVN3 (ABC) · .VnTime", DK_CHARSET_TCVN3},
            {"VNI Windows · VNI-Times", DK_CHARSET_VNI},
            {"VPS", DK_CHARSET_VPS},
            {"VISCII", DK_CHARSET_VISCII},
            {"VIQR", DK_CHARSET_VIQR},
            {"Windows CP 1258", DK_CHARSET_CP1258},
            {"NCR thập phân · &#7879;", DK_CHARSET_NCR_DEC},
            {"NCR thập lục · &#x1EC7;", DK_CHARSET_NCR_HEX},
        };
        for (const auto& c : charsets) cs->addItem(QString::fromUtf8(c.name), c.id);
        int curCs = store::get(L"Charset", store::get(L"OutputCombining", 0) ? DK_CHARSET_UNICODE_COMBINING
                                                                          : DK_CHARSET_UNICODE);
        cs->setCurrentIndex(qMax(0, cs->findData(curCs)));
        connect(cs, QOverload<int>::of(&QComboBox::currentIndexChanged), [cs](int) {
            int id = cs->currentData().toInt();
            store::set(L"Charset", id);
            store::set(L"OutputCombining", id == DK_CHARSET_UNICODE_COMBINING ? 1 : 0);
        });
        addRow(body, QStringLiteral("Bảng mã"),
               QStringLiteral("Cho văn bản cũ phông .VnTime, VNI-Times… Chữ Pāli vẫn ra Unicode"), cs, nullptr,
               QStringLiteral("Bảng mã quyết định <b>mã</b> của chữ được gõ ra.<br>Dùng <b>Unicode dựng sẵn</b> "
                              "cho mọi việc thông thường. Các bảng mã cũ (TCVN3, VNI, VPS…) chỉ để soạn tài liệu "
                              "dùng phông cũ, và phải chọn đúng phông đó."));
        v->addStretch(1);
    }

    // --- Trang 2: Pāli và Sanskrit ---
    void buildPaliPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Pāli và Sanskrit"));
        QVBoxLayout* body = makeCard(v);
        QComboBox* mdot = new QComboBox;
        mdot->addItem(QStringLiteral("ṃ  chấm dưới (VRI)"), 0);
        mdot->addItem(QStringLiteral("ṁ  chấm trên (PTS)"), 1);
        mdot->setCurrentIndex(store::get(L"PaliMDotAbove", 0) ? 1 : 0);
        addRow(body, QStringLiteral("Chữ niggahīta"), QStringLiteral("Cách viết âm mũi niggahīta trong Pāli"), mdot,
               nullptr, QStringLiteral("Niggahīta viết <b>ṃ</b> (chấm dưới, bản VRI) hoặc <b>ṁ</b> (chấm trên, bản PTS). "
                                       "Chọn một kiểu cho thống nhất; đổi ở đây là đổi cả phím Alt phải + m."));
        addRow(body, QStringLiteral("Chế độ Pāli (Velthuis)"),
               QStringLiteral("aa → ā · .t → ṭ · \"n → ṅ · ~n → ñ"), keycap(QStringLiteral("Ctrl + Shift + P")), nullptr,
               QStringLiteral("Bấm <b>Ctrl + Shift + P</b> để vào/ra chế độ Pāli. Trong chế độ này gõ chữ La-tinh "
                              "thường: <b>nibbaana</b> → nibbāna, <b>sa\"ngha</b> → saṅgha, <b>~naa.na</b> → ñāṇa. "
                              "Bảng đủ các tổ hợp ở bên dưới."));

        // Bảng phím AltGr: 10 chữ Pali + 4 chữ Sanskrit
        body = makeCard(v);
        addRow(body, QStringLiteral("Gõ nhanh bằng AltGr (Alt phải) + phím"),
               QStringLiteral("Dùng được ngay khi đang gõ tiếng Việt, không cần đổi chế độ"), nullptr, nullptr,
               QStringLiteral("Giữ phím <b>Alt bên phải</b> rồi bấm chữ trong bảng. Thêm <b>Shift</b> để ra chữ hoa "
                              "(Alt phải + Shift + a → Ā). Dùng được cả giữa từ: <b>nibb</b> + Alt phải + <b>a</b> + <b>na</b> → nibbāna."));
        QWidget* gridHost = new QWidget;
        QGridLayout* g = new QGridLayout(gridHost);
        g->setContentsMargins(0, 0, 0, 12);
        g->setHorizontalSpacing(6);
        g->setVerticalSpacing(6);
        // Bảng đọc từ core/include/dhammakey_pali_keys.h (nơi duy nhất định nghĩa phím Alt phải)
        for (int i = 0; i < DK_PALI_ALT_COUNT; ++i) {
            QLabel* chip = new QLabel(QString(QChar(kDkPaliAltKeys[i].key)) + QStringLiteral("  →  ") +
                                      QString::fromUtf8(kDkPaliAltKeys[i].lower));
            chip->setObjectName(kDkPaliAltKeys[i].sanskrit ? "chipSk" : "chip");
            chip->setAlignment(Qt::AlignCenter);
            if (kDkPaliAltKeys[i].sanskrit) chip->setToolTip(QStringLiteral("Sanskrit"));
            if (kDkPaliAltKeys[i].key == 'm') mChip_ = chip;
            g->addWidget(chip, i / 7, i % 7);
        }
        body->addWidget(gridHost);
        auto updateM = [this](int above) {
            if (mChip_) mChip_->setText(above ? QStringLiteral("m  →  ṁ") : QStringLiteral("m  →  ṃ"));
        };
        updateM(mdot->currentData().toInt());
        connect(mdot, QOverload<int>::of(&QComboBox::currentIndexChanged), [mdot, updateM](int) {
            int above = mdot->currentData().toInt();
            store::set(L"PaliMDotAbove", above);
            updateM(above);
        });
        QLabel* note = new QLabel(QStringLiteral("Ô viền đứt (ṣ ś ṛ ḥ) dùng cho Sanskrit."));
        note->setObjectName("hint");
        body->addWidget(note);
        body->addSpacing(6);

        // Bảng Velthuis (Ctrl + Shift + P): đủ các tổ hợp; ô viền đứt = Sanskrit
        body = makeCard(v);
        addRow(body, QStringLiteral("Bảng gõ Velthuis"),
               QStringLiteral("Trong chế độ Pāli (Ctrl + Shift + P). Ô viền đứt dùng cho Sanskrit"), nullptr, nullptr,
               QStringLiteral("Gõ chữ La-tinh thường; hai chữ ghép lại thành một chữ có dấu. Chữ hoa: <b>Aa</b> → Ā, "
                              "<b>.T</b> → Ṭ. Chữ kép gõ liền: <b>.t.t</b> → ṭṭ, <b>~n~n</b> → ññ."));
        QWidget* vHost = new QWidget;
        QGridLayout* vg = new QGridLayout(vHost);
        vg->setContentsMargins(0, 0, 0, 12);
        vg->setHorizontalSpacing(6);
        vg->setVerticalSpacing(6);
        const char* vpairs[][2] = {{"aa", "ā"}, {"ii", "ī"}, {"uu", "ū"}, {".m", "ṃ"}, {".n", "ṇ"}, {"\"n", "ṅ"},
                                   {"~n", "ñ"}, {".t", "ṭ"}, {".d", "ḍ"}, {".l", "ḷ"}, {".s", "ṣ"}, {"\"s", "ś"},
                                   {".r", "ṛ"}, {".rr", "ṝ"}, {".ll", "ḹ"}, {".h", "ḥ"}};
        for (int i = 0; i < 16; ++i) {
            QLabel* chip = new QLabel(QString::fromUtf8(vpairs[i][0]) + QStringLiteral("  →  ") +
                                      QString::fromUtf8(vpairs[i][1]));
            chip->setObjectName(i >= 10 ? "chipSk" : "chip");
            chip->setAlignment(Qt::AlignCenter);
            if (i >= 10) chip->setToolTip(QStringLiteral("Sanskrit"));
            vg->addWidget(chip, i / 8, i % 8);
        }
        body->addWidget(vHost);
        v->addStretch(1);
    }

    // --- Trang 3: Phím tắt ---
    void buildShortcutPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Phím tắt"));
        QVBoxLayout* body = makeCard(v);
        addRow(body, QStringLiteral("Bật / tắt tiếng Việt"),
               QStringLiteral("Hoặc bấm chuột trái vào biểu tượng V/E ở khay hệ thống"),
               keycap(QStringLiteral("Ctrl + `")));
        addRow(body, QStringLiteral("Bật / tắt gõ Pāli"), QStringLiteral("Chế độ Pāli Velthuis"),
               keycap(QStringLiteral("Ctrl + Shift + P")));
        addRow(body, QStringLiteral("Chọn kiểu gõ, mở cài đặt"),
               QStringLiteral("Bấm chuột phải vào biểu tượng ở khay hệ thống"), nullptr);
        v->addStretch(1);
    }

    // --- Trang 4: Bộ gõ khác ---
    // --- Trang: Tự tắt gõ dấu (0.3.32): bốn công tắc ứng với PasswordGuard, UrlGuard, ConsoleGuard, AsciiFieldGuard ---
    void buildGuardPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Tự tắt gõ dấu"));
        QLabel* lead = new QLabel(QStringLiteral(
            "Ở những ô sau, phím nào ra chữ nấy: không thêm dấu tiếng Việt, không đổi chữ Pāli, không đổi bảng mã. "
            "Rời ô là gõ dấu lại như cũ."));
        lead->setObjectName("hint");
        lead->setWordWrap(true);
        v->addWidget(lead);
        QVBoxLayout* body = makeCard(v);
        addToggle(body, QStringLiteral("Ô mật khẩu"),
                  QStringLiteral("Không bao giờ làm sai mật khẩu. Nên để bật"), L"PasswordGuard", 1, false,
                  QStringLiteral("Áp dụng cho ô mật khẩu và ô mã PIN, ở trình duyệt (kể cả cửa sổ ẩn danh) lẫn ứng dụng. "
                                 "Tắt: gõ <b>Tieengs</b> vào ô mật khẩu sẽ ra <b>Tiếng</b> — dễ gõ sai mật khẩu."));
        addToggle(body, QStringLiteral("Thanh địa chỉ web"),
                  QStringLiteral("Chrome, Edge, Firefox, Cốc Cốc… gõ địa chỉ web không bị thêm dấu"), L"UrlGuard", 1, false,
                  QStringLiteral("Địa chỉ web chỉ có chữ không dấu. Nếu bạn quen gõ <b>từ khoá tiếng Việt</b> ngay trên "
                                 "thanh địa chỉ để tìm kiếm, hãy <b>tắt</b> mục này để vẫn gõ được dấu ở đó."));
        addToggle(body, QStringLiteral("Cửa sổ dòng lệnh"),
                  QStringLiteral("Command Prompt, PowerShell, Windows Terminal…"), L"ConsoleGuard", 1, false,
                  QStringLiteral("Lệnh và đường dẫn không dùng dấu tiếng Việt, nên tự tắt để gõ lệnh không bị biến chữ."));
        addToggle(body, QStringLiteral("Ô email và ô số"),
                  QStringLiteral("Ô nhập email, số, số điện thoại: gõ chữ không dấu"), L"AsciiFieldGuard", 1, false,
                  QStringLiteral("Chỉ áp dụng cho ô mà ứng dụng khai báo là ô email, số hoặc điện thoại. "
                                 "Tắt nếu bạn cần gõ dấu ở những ô đó."));
        QLabel* note = new QLabel(QStringLiteral(
            "Biểu tượng ở khay hệ thống hiện trạng thái tắt khi đang ở những ô này. Đổi ở đây có hiệu lực ngay "
            "khi bạn quay lại gõ."));
        note->setObjectName("hint");
        note->setWordWrap(true);
        v->addWidget(note);
        v->addStretch(1);
    }

    void buildConflictPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Bộ gõ khác"));
        QVBoxLayout* body = makeCard(v);
        conflictBtn_ = new QPushButton(QStringLiteral("Tắt"));
        conflictBtn_->setObjectName("accent");
        addRow(body, QString(), QString(), conflictBtn_, &conflictLabel_);
        connect(conflictBtn_, &QPushButton::clicked, this, &SettingsWindow::stopOtherIme);
        addToggle(body, QStringLiteral("Nhắc khi có bộ gõ khác đang chạy"),
                  QStringLiteral("Nhắc tối đa một lần mỗi ngày"), L"ConflictNoticeOff", 0, /*inverted=*/true);
        QLabel* about = new QLabel(QStringLiteral(
            "UniKey, EVKey, GoTiengViet, OpenKey, Vietkey… chạy cùng lúc với DhammaKey sẽ làm chữ bị "
            "biến hai lần (gõ sai dấu). Khi tắt, chương trình đó không bị xóa; muốn dùng lại thì mở "
            "Task Manager → Startup → Enable."));
        about->setObjectName("hint");
        about->setWordWrap(true);
        v->addWidget(about);
        v->addStretch(1);
        refreshConflict();
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SettingsWindow::refreshConflict);
        timer->start(3000);
    }

    // --- Trang 5: Giao diện ---
    void buildSkinPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Giao diện"));
        // Ngôn ngữ giao diện (bước 1, 0.4.0): có hiệu lực khi mở lại cửa sổ cài đặt
        {
            QVBoxLayout* lb = makeCard(v);
            QComboBox* lang = new QComboBox;
            lang->addItem(QStringLiteral("Tiếng Việt"), 0);
            lang->addItem(QStringLiteral("English"), 1);
            lang->setCurrentIndex(store::get(L"UiLang", 0) ? 1 : 0);
            connect(lang, QOverload<int>::of(&QComboBox::currentIndexChanged), [lang](int) {
                store::set(L"UiLang", lang->currentData().toInt());
            });
            addRow(lb, QStringLiteral("Language"), QString(), lang);
        }
        QVBoxLayout* body = makeCard(v);
        addRow(body, QStringLiteral("Giao diện cửa sổ cài đặt"),
               QStringLiteral("Vừa: sáng, nhẹ nhàng · Tối: dịu mắt ban đêm · Cổ kính: màu giấy bối diệp"), nullptr);
        QWidget* tiles = new QWidget;
        QHBoxLayout* th = new QHBoxLayout(tiles);
        th->setContentsMargins(0, 4, 0, 14);
        th->setSpacing(14);
        QButtonGroup* group = new QButtonGroup(this);
        group->setExclusive(true);
        int cur = store::get(L"Skin", 0);
        for (int i = 0; i < 3; ++i) {
            SkinTile* tile = new SkinTile(i);
            tile->setChecked(i == cur);
            group->addButton(tile, i);
            th->addWidget(tile);
        }
        th->addStretch(1);
        body->addWidget(tiles);
        connect(group, &QButtonGroup::idClicked, [this](int id) {
            store::set(L"Skin", id);
            applySkin(id);
        });

        // Bộ chữ trên tray icon (registry TrayGlyph). Bộ gõ đọc lại khi người dùng quay về ô gõ chữ.
        body = makeCard(v);
        QComboBox* glyph = new QComboBox;
        glyph->addItem(QStringLiteral("Chữ Latinh:  V · P · E"), 0);
        glyph->addItem(QStringLiteral("Chữ bản địa:  Vi · Pā · EN"), 1);
        glyph->setCurrentIndex(store::get(L"TrayGlyph", store::defaultTrayGlyph()) ? 1 : 0);
        connect(glyph, QOverload<int>::of(&QComboBox::currentIndexChanged), [glyph](int) {
            store::set(L"TrayGlyph", glyph->currentData().toInt());
        });
        addRow(body, QStringLiteral("Chữ trên biểu tượng ở khay hệ thống"),
               QStringLiteral("Chữ bản địa theo văn tự của kiểu gõ: Vi, Pā, अ, က, ก, 中, 한, あ, 喃. "
                              "Đổi xong, bấm vào chỗ gõ chữ là biểu tượng đổi theo"),
               glyph);
        v->addStretch(1);
    }

    // --- Trang 6: Cập nhật ---
    void buildUpdatePage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Cập nhật"));
        QVBoxLayout* body = makeCard(v);
        updateBtn_ = new QPushButton(QStringLiteral("Kiểm tra cập nhật"));
        updateBtn_->setObjectName("accent");
        addRow(body, QStringLiteral("Đang dùng DhammaKey %1").arg(QStringLiteral(DK_VERSION)), QString(),
               updateBtn_, &updateTitle_);
        connect(updateBtn_, &QPushButton::clicked, this, [this] {
            if (pending_.ok && newerThan(pending_.version, QStringLiteral(DK_VERSION))) downloadAndInstall();
            else checkUpdate();
        });
        updateStatus_ = new QLabel;
        updateStatus_->setObjectName("hint");
        updateStatus_->setWordWrap(true);
        updateStatus_->hide();
        body->addWidget(updateStatus_);
        updateProgress_ = new QProgressBar;
        updateProgress_->setTextVisible(false);
        updateProgress_->hide();
        body->addWidget(updateProgress_);
        body->addSpacing(8);
        addToggle(body, QStringLiteral("Tự kiểm tra cập nhật mỗi tuần"),
                  QStringLiteral("Khi có bản mới, DhammaKey hỏi trước khi tải về"), L"AutoUpdateCheck", 1);

        // Có gì mới: nhật ký từng phiên bản (settings/whatsnew.md, nhúng trong tài nguyên), cuộn trong thẻ
        v->addSpacing(18);
        QLabel* newsHead = new QLabel(QStringLiteral("Có gì mới"));
        newsHead->setObjectName("sectionTitle");
        v->addWidget(newsHead);
        v->addSpacing(8);
        QVBoxLayout* news = makeCard(v);
        news->setContentsMargins(18, 10, 10, 10);
        QTextBrowser* log = new QTextBrowser;
        log->setObjectName("whatsNew");
        log->setOpenExternalLinks(true);
        log->setFrameShape(QFrame::NoFrame);
        log->setMinimumHeight(160);
        QFile f(QStringLiteral(":/whatsnew.md"));
        if (f.open(QIODevice::ReadOnly)) log->setMarkdown(QString::fromUtf8(f.readAll()));
        news->addWidget(log);
        v->setStretchFactor(news->parentWidget(), 1);
    }

    void setUpdateStatus(const QString& text) {
        updateStatus_->setText(text);
        updateStatus_->setVisible(!text.isEmpty());
    }

    void checkUpdate() {
        updateBtn_->setEnabled(false);
        setUpdateStatus(QStringLiteral("Đang kiểm tra…"));
        QPointer<SettingsWindow> self(this);
        std::thread([self] {
            UpdateInfo u = fetchUpdateInfo();
            QMetaObject::invokeMethod(qApp, [self, u] {
                if (!self) return;
                self->updateBtn_->setEnabled(true);
                if (!u.ok) {
                    // Máy chủ chưa có thông tin bản mới thì chỉ báo tin, không coi là lỗi (0.3.19)
                    self->setUpdateStatus(u.notice ? u.error
                                                   : QStringLiteral("Không kiểm tra được: %1").arg(u.error));
                    return;
                }
                self->pending_ = u;
                if (newerThan(u.version, QStringLiteral(DK_VERSION))) {
                    self->updateTitle_->setText(QStringLiteral("Có bản mới: DhammaKey %1").arg(u.version));
                    self->setUpdateStatus(u.notes.isEmpty() ? QString() : QStringLiteral("Thay đổi: %1").arg(u.notes));
                    self->updateBtn_->setText(QStringLiteral("Tải và cài"));
                } else {
                    self->setUpdateStatus(QStringLiteral("Đây là bản mới nhất."));
                }
            }, Qt::QueuedConnection);
        }).detach();
    }

    void downloadAndInstall() {
        updateBtn_->setEnabled(false);
        updateProgress_->setRange(0, 0);
        updateProgress_->show();
        setUpdateStatus(QStringLiteral("Đang tải DhammaKey %1…").arg(pending_.version));
        QPointer<SettingsWindow> self(this);
        UpdateInfo u = pending_;
        std::thread([self, u] {
            net::Response r = net::get(u.url, [self](qint64 done, qint64 total) {
                QMetaObject::invokeMethod(qApp, [self, done, total] {
                    if (!self) return;
                    if (total > 0) { self->updateProgress_->setRange(0, 1000); self->updateProgress_->setValue(int(done * 1000 / total)); }
                }, Qt::QueuedConnection);
                return true;
            });
            QString error = r.error, file;
            if (error.isEmpty() &&
                QCryptographicHash::hash(r.body, QCryptographicHash::Sha256).toHex() != u.sha256.toLatin1())
                error = QStringLiteral("Tệp tải về bị hỏng hoặc không đúng (sai mã kiểm tra). Hãy thử lại.");
            if (error.isEmpty()) {
                file = QDir::temp().filePath(QStringLiteral("DhammaKey-%1-x64.msi").arg(u.version));
                QSaveFile f(file);
                if (!f.open(QIODevice::WriteOnly) || f.write(r.body) != r.body.size() || !f.commit())
                    error = QStringLiteral("Không lưu được tệp cài đặt.");
            }
            QMetaObject::invokeMethod(qApp, [self, error, file] {
                if (!self) return;
                self->updateProgress_->hide();
                self->updateBtn_->setEnabled(true);
                if (!error.isEmpty()) { self->setUpdateStatus(error); return; }
                self->setUpdateStatus(QStringLiteral("Đã tải xong. Đang mở bộ cài…"));
                // Bộ cài tự gỡ bản cũ (MajorUpgrade); đóng cửa sổ này để không giữ tệp
                QDesktopServices::openUrl(QUrl::fromLocalFile(file));
                QTimer::singleShot(800, qApp, &QApplication::quit);
            }, Qt::QueuedConnection);
        }).detach();
    }

    // --- Trang 7: Góp ý ---
    void buildFeedbackPage() {
        QVBoxLayout* v = makePage(stack_, QStringLiteral("Góp ý"));
        QVBoxLayout* body = makeCard(v);
        fbKind_ = new QComboBox;
        fbKind_->addItems({QStringLiteral("Góp ý"), QStringLiteral("Báo lỗi"), QStringLiteral("Đề nghị tính năng")});
        addRow(body, QStringLiteral("Loại"), QString(), fbKind_);
        addSeparator(body);
        body->addSpacing(10);
        fbText_ = new QPlainTextEdit;
        fbText_->setPlaceholderText(QStringLiteral("Xin ghi góp ý, lỗi gặp phải (gõ chữ gì, ở phần mềm nào, ra kết quả gì)…"));
        fbText_->setMinimumHeight(120);
        body->addWidget(fbText_);
        body->addSpacing(8);
        fbContact_ = new QLineEdit;
        fbContact_->setPlaceholderText(QStringLiteral("Email hoặc cách liên hệ (không bắt buộc)"));
        body->addWidget(fbContact_);
        body->addSpacing(4);
        fbSystem_ = addToggle(body, QStringLiteral("Gửi kèm phiên bản DhammaKey và Windows"),
                              QStringLiteral("Giúp tìm lỗi nhanh hơn. Không gửi thông tin cá nhân nào khác"),
                              L"FeedbackSendSystem", 1);
        QWidget* actions = new QWidget;
        QHBoxLayout* ah = new QHBoxLayout(actions);
        ah->setContentsMargins(0, 6, 0, 12);
        fbStatus_ = new QLabel;
        fbStatus_->setObjectName("hint");
        fbStatus_->setWordWrap(true);
        ah->addWidget(fbStatus_, 1);
        fbSend_ = new QPushButton(QStringLiteral("Gửi góp ý"));
        fbSend_->setObjectName("accent");
        ah->addWidget(fbSend_);
        body->addWidget(actions);
        connect(fbSend_, &QPushButton::clicked, this, &SettingsWindow::sendFeedback);
        v->addStretch(1);
    }

    void sendFeedback() {
        QString text = fbText_->toPlainText().trimmed();
        if (text.isEmpty()) { fbStatus_->setText(QStringLiteral("Xin ghi nội dung góp ý.")); return; }
        if (QStringLiteral(DK_FEEDBACK_FORM_ID) == QStringLiteral("FORM_ID")) {
            fbStatus_->setText(QStringLiteral("Mục góp ý chưa được thiết lập (thiếu mã Google Form)."));
            return;
        }
        auto enc = [](const QString& s) { return QUrl::toPercentEncoding(s); };
        QByteArray data;
        data += DK_FEEDBACK_ENTRY_KIND "=" + enc(fbKind_->currentText());
        data += "&" DK_FEEDBACK_ENTRY_TEXT "=" + enc(text);
        data += "&" DK_FEEDBACK_ENTRY_CONTACT "=" + enc(fbContact_->text().trimmed());
        if (fbSystem_->isChecked())
            data += "&" DK_FEEDBACK_ENTRY_SYSTEM "=" +
                    enc(QStringLiteral("DhammaKey %1 · %2 (%3) · %4")
                            .arg(QStringLiteral(DK_VERSION), QSysInfo::prettyProductName(),
                                 QSysInfo::kernelVersion(), QSysInfo::currentCpuArchitecture()));
        QString url = QStringLiteral("https://docs.google.com/forms/d/e/%1/formResponse").arg(DK_FEEDBACK_FORM_ID);
        fbSend_->setEnabled(false);
        fbStatus_->setText(QStringLiteral("Đang gửi…"));
        QPointer<SettingsWindow> self(this);
        std::thread([self, url, data] {
            net::Response r = net::postForm(url, data);
            QMetaObject::invokeMethod(qApp, [self, r] {
                if (!self) return;
                self->fbSend_->setEnabled(true);
                if (r.ok()) {
                    self->fbText_->clear();
                    self->fbStatus_->setText(QStringLiteral("Đã gửi. Xin cảm ơn!"));
                } else {
                    self->fbStatus_->setText(QStringLiteral("Chưa gửi được: %1").arg(r.error));
                }
            }, Qt::QueuedConnection);
        }).detach();
    }

private slots:
    void refreshConflict() {
#ifdef _WIN32
        if (!conflictLabel_) return;
        std::wstring name = conflict::FindRunning();
        if (name.empty()) {
            conflictLabel_->setText(QStringLiteral("Không có bộ gõ tiếng Việt nào khác đang chạy."));
            conflictLabel_->setObjectName("");
            conflictBtn_->hide();
        } else {
            other_ = QString::fromStdWString(name);
            conflictLabel_->setText(QStringLiteral("%1 đang chạy, có thể làm gõ sai dấu.").arg(other_));
            conflictLabel_->setObjectName("warn");
            conflictBtn_->setText(QStringLiteral("Tắt %1").arg(other_));
            conflictBtn_->show();
        }
        conflictLabel_->style()->unpolish(conflictLabel_);
        conflictLabel_->style()->polish(conflictLabel_);
#endif
    }

    void stopOtherIme() {
#ifdef _WIN32
        auto ans = QMessageBox::question(
            this, QStringLiteral("DhammaKey"),
            QStringLiteral("Tắt %1 và không cho nó tự chạy khi mở máy?\n\n"
                           "%1 không bị xóa. Muốn dùng lại: Task Manager → Startup → %1 → Enable.").arg(other_));
        if (ans != QMessageBox::Yes) return;
        conflict::Result r = conflict::DisableAndStop();
        if (r.failed || r.stopFailed)
            QMessageBox::warning(this, QStringLiteral("DhammaKey"),
                                 QStringLiteral("Chưa tắt được hết %1 (cần quyền quản trị).\n\n"
                                                "Hãy mở Task Manager → Startup → %1 → Disable, rồi thoát %1 "
                                                "ở khay hệ thống.").arg(other_));
        refreshConflict();
#endif
    }

protected:
#ifdef _WIN32
    // Bỏ thanh tiêu đề của Windows nhưng giữ khung: Qt coi là cửa sổ không viền (FramelessWindowHint),
    // rồi trả lại các kiểu WS_THICKFRAME/WS_CAPTION để vẫn kéo giãn, Snap, thu nhỏ/phóng to có hiệu ứng.
    void setupCustomFrame() {
        setWindowFlags((windowFlags() | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                        Qt::WindowMinimizeButtonHint) & ~Qt::WindowMaximizeButtonHint);
        HWND hwnd = (HWND)winId();
        LONG style = GetWindowLongW(hwnd, GWL_STYLE);
        // Không có WS_MAXIMIZEBOX: nhấp đúp thanh tiêu đề, kéo lên mép màn hình, Win+↑ đều không phóng to.
        // Giữ WS_THICKFRAME chỉ để Windows vẽ bóng đổ; mép không kéo giãn được (xem WM_NCHITTEST).
        SetWindowLongW(hwnd, GWL_STYLE, (style | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) &
                                            ~WS_MAXIMIZEBOX);
        updateShadow();
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    static bool dwmOn() {
        BOOL on = FALSE;
        return SUCCEEDED(DwmIsCompositionEnabled(&on)) && on;
    }
    void updateShadow() {
        // Kéo khung DWM vào 1 px để Windows vẫn vẽ bóng đổ và viền (Win7 Aero trở lên)
        if (!dwmOn()) return;
        MARGINS m = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea((HWND)winId(), &m);
    }
    // Bề dày khung (px thật) — khi phóng to, Windows đẩy khung ra ngoài màn hình đúng bằng chừng này
    static int frameThickness() {
        return GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    }
    void syncMaximized() {
        // Phóng to: chừa lề bằng bề dày khung để nội dung không bị cắt mất ở mép màn hình
        int m = isMaximized() ? qRound(frameThickness() / devicePixelRatioF()) : 0;
        layout()->setContentsMargins(m, m, m, m);
        if (titleBar_) titleBar_->syncState();
    }
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::WindowStateChange) syncMaximized();
        if (e->type() == QEvent::ActivationChange && titleBar_) titleBar_->syncState();
        QWidget::changeEvent(e);
    }
    bool nativeEvent(const QByteArray& type, void* message, long* result) override {
        MSG* msg = static_cast<MSG*>(message);
        switch (msg->message) {
        case WM_NCCALCSIZE:
            if (msg->wParam) {  // cả cửa sổ là vùng của mình, không còn thanh tiêu đề Windows
                *result = 0;
                return true;
            }
            break;
        case WM_NCACTIVATE:
            // Máy tắt hiệu ứng Aero (Win7 Basic/Classic): chặn Windows vẽ đè thanh tiêu đề cũ lên cửa sổ
            if (!dwmOn()) {
                *result = DefWindowProcW(msg->hwnd, msg->message, msg->wParam, -1);
                return true;
            }
            break;
        case WM_DWMCOMPOSITIONCHANGED:
            updateShadow();
            break;
        case WM_NCHITTEST: {
            RECT wr;
            GetWindowRect(msg->hwnd, &wr);
            const int x = GET_X_LPARAM(msg->lParam) - wr.left, y = GET_Y_LPARAM(msg->lParam) - wr.top;
            // Thanh tiêu đề (trừ hai nút): kéo để dời cửa sổ, chuột phải ra menu hệ thống. Mép cửa sổ là HTCLIENT nên không kéo giãn được.
            if (titleBar_) {
                const qreal dpr = devicePixelRatioF();
                QPoint pt = titleBar_->mapFrom(this, QPoint(qRound(x / dpr), qRound(y / dpr)));
                if (titleBar_->rect().contains(pt) && !titleBar_->childAt(pt)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
            *result = HTCLIENT;
            return true;
        }
        default:
            break;
        }
        return QWidget::nativeEvent(type, message, result);
    }
    TitleBar* titleBar_ = nullptr;
#endif

private:
    QListWidget* nav_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QLabel* conflictLabel_ = nullptr;
    QPushButton* conflictBtn_ = nullptr;
    QLabel* mChip_ = nullptr;
    QString other_;
    // Cập nhật
    QPushButton* updateBtn_ = nullptr;
    QLabel* updateTitle_ = nullptr;
    QLabel* logo_ = nullptr;
    QLabel* navBrand_ = nullptr;   // macOS/Linux: chữ DhammaKey ở đầu cột trái
    QLabel* navSlogan_ = nullptr;
    EmbossWheel* wheel_ = nullptr;
    QLabel* updateStatus_ = nullptr;
    QProgressBar* updateProgress_ = nullptr;
    UpdateInfo pending_;
    // Góp ý
    QComboBox* fbKind_ = nullptr;
    QPlainTextEdit* fbText_ = nullptr;
    QLineEdit* fbContact_ = nullptr;
    Toggle* fbSystem_ = nullptr;
    QLabel* fbStatus_ = nullptr;
    QPushButton* fbSend_ = nullptr;
};

// Bộ gõ gọi "DhammaKeySettings.exe --check-update" mỗi tuần một lần: kiểm tra lặng lẽ,
// chỉ hiện cửa sổ khi có bản mới và người dùng đồng ý xem.
// ---------------- Cửa sổ "Về chúng tôi" (bản 0.3.9) ----------------
// Mở từ dòng cuối menu chuột phải ở khay hệ thống: DhammaKeySettings.exe --about
// Giới thiệu ngắn (khung chữ khoảng 10 × 10 cm ở 100%, hơn 36 cm²; chỉ nói về DhammaKey và DhammaWare, không nhắc thương hiệu khác), dưới là bánh xe cam DhammaWare và phiên bản.
// Hiệu ứng nổi (emboss): bánh xe và chữ DhammaWare có vệt sáng phía trên-trái, bóng phía dưới-phải;
// khung giới thiệu có viền vát (sáng trên-trái, tối dưới-phải).
static const wchar_t kAboutTitle[] = L"Về DhammaKey";  // tên cửa sổ (thanh tác vụ, Alt+Tab); đổi từ "Về chúng tôi — DhammaKey" ở 0.3.19

class EmbossText : public QWidget {
public:
    EmbossText(const QString& text, const QFont& f, QWidget* parent = nullptr) : QWidget(parent), text_(text), f_(f) {
        QFontMetrics fm(f_);
        setFixedSize(fm.horizontalAdvance(text_) + 6, fm.height() + 4);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setFont(f_);
        EmbossColors ec = embossColors(g_theme);
        QRectF r = QRectF(rect()).adjusted(3, 2, -3, -2);
        p.setPen(ec.lo);
        p.drawText(r.translated(1.2, 1.2), Qt::AlignCenter, text_);
        p.setPen(ec.hi);
        p.drawText(r.translated(-1, -1), Qt::AlignCenter, text_);
        p.setPen(g_theme.text);
        p.drawText(r, Qt::AlignCenter, text_);
    }
private:
    QString text_;
    QFont f_;
};

class AboutWindow : public QWidget {
public:
    AboutWindow() {
        setWindowTitle(QString::fromWCharArray(kAboutTitle));
        setObjectName("aboutRoot");
        setFixedWidth(440);    // chiều cao tính theo nội dung ở cuối hàm dựng (0.3.16)
        const Theme& t = g_theme;
        QPalette pal = palette();
        pal.setColor(QPalette::Window, t.bg);
        setPalette(pal);
        setAutoFillBackground(true);
        // Viền vát cho khung giới thiệu: sáng ở trên-trái, tối ở dưới-phải
        QString lite = t.dark ? "#3d3d3d" : "#ffffff";
        QString deep = t.dark ? "#0e0e0e" : t.border.darker(125).name();
        setStyleSheet(QStringLiteral(
            "QWidget { font-family: \"%1\"; color: %2; }"
            "#aboutHead { font-family: \"%3\"; font-size: 20pt; font-weight: 700; }"
            "#aboutPanel { background: %4; border: 1px solid; border-radius: 8px;"
            "  border-top-color: %5; border-left-color: %5; border-bottom-color: %6; border-right-color: %6; }"
            "#aboutText { font-size: 10.5pt; background: transparent; }"
            "#aboutVer { color: %7; font-size: 9pt; }"
            "#aboutLegal { color: %7; font-size: 8pt; }"
            "#aboutSlogan { color: %8; font-size: 10pt; }"
            "#aboutLinks { color: %7; font-size: 8pt; }")
                          .arg(t.font, t.text.name(), t.titleFont, t.card.name(), lite, deep, t.hint.name(),
                               sloganCss(t)));

        QVBoxLayout* v = new QVBoxLayout(this);
        v->setContentsMargins(32, 28, 32, 26);
        v->setSpacing(0);

        // Tiêu đề: logo DhammaKey (phím ṃ) đứng trước chữ "Về chúng tôi" (người dùng yêu cầu, bản 0.3.16)
        QHBoxLayout* headRow = new QHBoxLayout;
        headRow->setSpacing(10);
        headRow->addStretch(1);
        QLabel* headLogo = new QLabel;
        headLogo->setPixmap(logoPixmap(34, devicePixelRatioF()));
        headLogo->setFixedSize(34, 34);
        headRow->addWidget(headLogo, 0, Qt::AlignVCenter);
        QLabel* head = new QLabel(QStringLiteral("Về DhammaKey\u2122"));  // đổi từ "Về chúng tôi" (theo yêu cầu 15/11/2026, 0.3.19)
        head->setObjectName("aboutHead");
        headRow->addWidget(head, 0, Qt::AlignVCenter);
        headRow->addStretch(1);
        v->addLayout(headRow);
        // Slogan đi kèm logo DhammaKey (bộ nhận diện 1.4), ở giữa, ngay dưới tiêu đề
        v->addSpacing(4);
        QLabel* slogan = new QLabel(QStringLiteral(DK_SLOGAN));
        slogan->setObjectName("aboutSlogan");
        v->addWidget(slogan, 0, Qt::AlignHCenter);
        v->addSpacing(14);

        QFrame* panel = new QFrame;
        panel->setObjectName("aboutPanel");
        panel->setMinimumSize(376, 250);   // ≈ 10 × 6,6 cm ở 100% (96 dpi)
        QVBoxLayout* pv = new QVBoxLayout(panel);
        pv->setContentsMargins(20, 16, 20, 16);
        QLabel* text = new QLabel(uiEnglish() ? QString::fromUtf8(DK_ABOUT_EN_TEXT) : QStringLiteral(
            // Lời giới thiệu (người dùng đưa nguyên văn và duyệt ảnh xem trước 14/11/2026, bản 0.3.15; bỏ kệ Pháp Cú)
            "<p><b>DhammaKey™</b> là bộ gõ đa ngôn ngữ miễn phí được phát triển bởi <b>DhammaWare™</b>, hướng đến "
            "nhu cầu nhập liệu học thuật, nghiên cứu, dịch thuật, giáo dục, Phật học và sử dụng ngôn ngữ quốc tế "
            "trên nhiều nền tảng.</p>"
            "<p>Ứng dụng từng bước hỗ trợ gõ thuận tiện các hệ chữ và ngôn ngữ quan trọng như Tiếng Việt, Pāli, "
            "Sanskrit, Miến Điện (Myanmar), Thái Lan, Trung Hoa, Hàn Quốc, Nhật Bản, Hán Nôm cùng nhiều kiểu nhập "
            "liệu truyền thống của từng quốc gia. DhammaKey được thiết kế để tôn trọng chuẩn bản địa, giúp người dùng "
            "làm việc theo đúng thói quen và quy tắc nhập liệu vốn có của từng ngôn ngữ.</p>"));
        text->setObjectName("aboutText");
        text->setWordWrap(true);
        text->setTextFormat(Qt::RichText);
        pv->addWidget(text);
        v->addWidget(panel);   // khung vừa đủ chữ, không còn khoảng trống bên dưới (0.3.16)
        v->addSpacing(22);

        EmbossWheel* wheel = new EmbossWheel(96);
        v->addWidget(wheel, 0, Qt::AlignHCenter);
        v->addSpacing(8);
        QFont bf(t.titleFont);
        bf.setPointSizeF(17);
        bf.setWeight(QFont::Bold);
        v->addWidget(new EmbossText(QStringLiteral("DhammaWare™"), bf), 0, Qt::AlignHCenter);
        QLabel* ver = new QLabel((uiEnglish() ? QStringLiteral(DK_ABOUT_EN_VERSION) : QStringLiteral("DhammaKey™ · Phiên bản %1 · dāna")).arg(QStringLiteral(DK_VERSION)));
        ver->setObjectName("aboutVer");
        v->addWidget(ver, 0, Qt::AlignHCenter);
        v->addSpacing(10);
        // Bản quyền và nhãn hiệu (chốt 14/11/2026): © cho mã và hình; ™ cho hai tên (chưa đăng ký nên không dùng ®)
        QLabel* legal = new QLabel((uiEnglish() ? QStringLiteral(DK_ABOUT_EN_LEGAL) : QStringLiteral("© %1 %2<br>DhammaKey™, DhammaPāṭha™ và DhammaWare™ thuộc về DhammaWare."))
                                       .arg(QStringLiteral(DK_COPYRIGHT_YEAR), QStringLiteral(DK_COPYRIGHT_HOLDER)));
        legal->setObjectName("aboutLegal");
        legal->setAlignment(Qt::AlignHCenter);
        v->addWidget(legal, 0, Qt::AlignHCenter);
        // Giấy phép (bản 0.3.18): điều khoản của DhammaKey (PolyForm Noncommercial 1.0.0) và giấy phép
        // của thành phần bên thứ ba (Qt, phông chữ…). Các tệp .txt nằm cạnh DhammaKeySettings.exe.
        v->addSpacing(6);
        QLabel* links = new QLabel(QStringLiteral(
            "<a href=\"LICENSE.vi.txt\" style=\"color:%1; text-decoration:none\">Điều khoản sử dụng</a>"
            " &nbsp;·&nbsp; "
            "<a href=\"THIRD-PARTY-NOTICES.txt\" style=\"color:%1; text-decoration:none\">Giấy phép và ghi công bên thứ ba</a>")
                                       .arg(t.accent.name()));
        links->setObjectName("aboutLinks");
        links->setTextFormat(Qt::RichText);
        links->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        links->setAlignment(Qt::AlignHCenter);
        connect(links, &QLabel::linkActivated, this, [](const QString& file) {
            const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(file);
            if (QFileInfo::exists(path)) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            else QDesktopServices::openUrl(QUrl(QStringLiteral("https://dhammaware.org")));
        });
        v->addWidget(links, 0, Qt::AlignHCenter);
        links_ = links;

        // Cửa sổ cao vừa đủ nội dung (lời Việt hay Anh dài ngắn khác nhau), không để khoảng trống
        ensurePolished();
        int h = heightForWidth(440);
        if (h <= 0) h = sizeHint().height();
        setFixedHeight(h);

#ifdef _WIN32
        // Khung kiểu Windows 11 như Bảng điều khiển: không thanh tiêu đề hệ thống, có bóng đổ; kéo ở bất kỳ chỗ trống nào
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
        HWND hwnd = (HWND)winId();
        LONG style = GetWindowLongW(hwnd, GWL_STYLE);
        SetWindowLongW(hwnd, GWL_STYLE, (style | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU) & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX));
        BOOL dwm = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&dwm)) && dwm) {
            MARGINS m = {1, 1, 1, 1};
            DwmExtendFrameIntoClientArea(hwnd, &m);
        }
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        close_ = new CaptionButton(CaptionButton::Close, this);
        close_->setToolTip(QStringLiteral("Đóng"));
        close_->move(width() - kCapBtnW, 0);
        connect(close_, &QAbstractButton::clicked, this, &QWidget::close);
#endif
    }
protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Escape) close();
        else QWidget::keyPressEvent(e);
    }
#ifdef _WIN32
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::ActivationChange && close_) close_->update();
        QWidget::changeEvent(e);
    }
    bool nativeEvent(const QByteArray& type, void* message, long* result) override {
        MSG* msg = static_cast<MSG*>(message);
        switch (msg->message) {
        case WM_NCCALCSIZE:
            if (msg->wParam) { *result = 0; return true; }
            break;
        case WM_NCACTIVATE: {
            BOOL on = FALSE;
            if (!(SUCCEEDED(DwmIsCompositionEnabled(&on)) && on)) {
                *result = DefWindowProcW(msg->hwnd, msg->message, msg->wParam, -1);
                return true;
            }
            break;
        }
        case WM_NCHITTEST: {
            RECT wr;
            GetWindowRect(msg->hwnd, &wr);
            const qreal dpr = devicePixelRatioF();
            QPoint pt(qRound((GET_X_LPARAM(msg->lParam) - wr.left) / dpr), qRound((GET_Y_LPARAM(msg->lParam) - wr.top) / dpr));
            const bool onButton = (close_ && close_->geometry().contains(pt)) || (links_ && links_->geometry().contains(pt));
            *result = onButton ? HTCLIENT : HTCAPTION;
            return true;
        }
        default:
            break;
        }
        return QWidget::nativeEvent(type, message, result);
    }
    CaptionButton* close_ = nullptr;
    QLabel* links_ = nullptr;
#endif
};

static int runAbout(int argc, char* argv[]) {
#ifdef _WIN32
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\DhammaKeyAbout");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND w = FindWindowW(nullptr, kAboutTitle)) { ShowWindow(w, SW_RESTORE); SetForegroundWindow(w); }
        return 0;
    }
#endif
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    g_theme = makeTheme(store::get(L"Skin", 0));
    AboutWindow w;
    w.show();
    return app.exec();
}

static int silentUpdateCheck(int argc, char* argv[]) {
    QApplication app(argc, argv);
    UpdateInfo u = fetchUpdateInfo();
    if (!u.ok || !newerThan(u.version, QStringLiteral(DK_VERSION))) return 0;
    app.setStyle("Fusion");
    QMessageBox box(QMessageBox::Information, QStringLiteral("DhammaKey"),
                    QStringLiteral("Đã có DhammaKey %1 (máy đang dùng %2).").arg(u.version, QStringLiteral(DK_VERSION)),
                    QMessageBox::Yes | QMessageBox::No);
    if (!u.notes.isEmpty()) box.setInformativeText(QStringLiteral("Thay đổi: %1\n\nCập nhật ngay?").arg(u.notes));
    else box.setInformativeText(QStringLiteral("Cập nhật ngay?"));
    box.setWindowFlag(Qt::WindowStaysOnTopHint);
    if (box.exec() != QMessageBox::Yes) return 0;
    SettingsWindow w;
    w.applySkin(store::get(L"Skin", 0));
    w.openPage(SettingsWindow::PAGE_UPDATE);
    w.show();
    w.checkUpdateNow();
    return app.exec();
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == QStringLiteral("--about")) {
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
            QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
            QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
            return runAbout(argc, argv);
        }
        if (QString::fromLocal8Bit(argv[i]) == QStringLiteral("--check-update")) {
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
            return silentUpdateCheck(argc, argv);
        }
    }
#ifdef _WIN32
    // Chỉ mở một cửa sổ: nếu đã mở rồi thì đưa nó lên trước
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\DhammaKeySettings");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND w = FindWindowW(nullptr, L"Bảng điều khiển DhammaKey");
        if (w) {
            ShowWindow(w, SW_RESTORE);
            SetForegroundWindow(w);
        }
        return 0;
    }
#endif
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    SettingsWindow w;
    w.applySkin(store::get(L"Skin", 0));
    w.show();
#ifdef _WIN32
    w.applySkin(store::get(L"Skin", 0));  // lần nữa sau khi cửa sổ đã có trên màn hình (thanh tiêu đề tối)
#endif
    return app.exec();
}

#include "main.moc"
