// SPDX-License-Identifier: MIT
#pragma once

#include <QtWidgets>
#include <functional>
#include "calculatorthemes.h"
#include "matheditor.h"
#include "keypaddesigner.h"
#include <optional>
class ArgumentGuide;

struct KeyDefinition {
    QString label, token, secondary, alternate;
    int kind = 0; // 1 number, 2 evaluate, 3 clear/delete, 4 shift, 5 alpha
    QString alphaLabel, alphaToken;
    // Keep the key position and its secondary functions when the selected
    // radix cannot accept its normal digit.
    bool primaryAvailable = true;
};

// A real accessible button, with a deliberately painted handheld face.
// Animations are also available to the expression editor's keyboard handler.
class CalculatorKey final : public QPushButton {
public:
    explicit CalculatorKey(KeyDefinition definition, QWidget *parent = nullptr);
    QString token() const;
    QString primaryToken() const { return definition_.token; }
    QString alternateToken() const { return definition_.alternate; }
    QString alphaToken() const { return definition_.alphaToken; }
    bool primaryAvailable() const { return definition_.primaryAvailable; }
    const KeyDefinition &definition() const { return definition_; }
    void setDefinition(KeyDefinition definition);
    void setModes(bool shift, bool alpha);
    void setLook(const QString &design);
    void setCalculatorTheme(const QString &theme);
    void setTranslatedLabel(const QString &label);
    void setDecimalSeparator(QChar separator);
    void setDesktopEffects(int effects);
    void flash();
    bool isAnimating() const { return animation_.state() == QAbstractAnimation::Running || depression_ > 0.01; }
    QSize sizeHint() const override { return QSize(62, 43); }
    QSize minimumSizeHint() const override { return QSize(26, 24); }
protected:
    bool event(QEvent *) override;
    void paintEvent(QPaintEvent *) override;
private:
    KeyDefinition definition_;
    QString design_ = "Desktop", theme_ = "Classic", translatedLabel_;
    bool shifted_ = false, alphaSelected_ = false, active_ = false;
    QChar decimalSeparator_ = '.';
    int desktopEffects_ = 0;
    qreal depression_ = 0;
    QVariantAnimation animation_;
    void animate(qreal target, int duration);
};

class CalculatorPanel final : public QWidget {
public:
    explicit CalculatorPanel(QWidget *parent = nullptr);
    MathEditor *editor() const { return editor_; }
    MathEditor *resultView() const { return result_; }
    QLabel *approximateLabel() const { return approximate_; }
    QComboBox *angleBox() const { return angle_; }
    QComboBox *exactBox() const { return exact_; }
    QComboBox *baseBox() const { return base_; }
    QComboBox *notationBox() const { return notation_; }
    QList<QLabel *> modeIndicators() const { return {angleIndicator_,exactIndicator_,baseIndicator_,notationIndicator_,numberKind_,memoryIndicator_}; }
    void syncIndicators();
    void setDecimalSeparator(QChar separator);
    QChar decimalSeparator() const { return decimalSeparator_; }
    void setStructuredInput(bool structured);
    void setDesktopEffects(int effects);
    void setKeyTooltipsEnabled(bool enabled);
    bool keyTooltipsEnabled() const { return keyTooltipsEnabled_; }
    void setScreenTooltipsEnabled(bool enabled);
    bool screenTooltipsEnabled() const { return screenTooltipsEnabled_; }
    QList<CalculatorKey *> keyButtons() const;
    QList<QPushButton *> memoryButtons() const { return memories_; }
    QPushButton *copyButton() const { return copy_; }
    QPushButton *settingsButton() const { return settings_; }
    QLabel *statusLabel() const { return status_; }
    void setGearIcon(const QIcon &icon);
    void setStatus(QString status);
    void setApproximation(QString text, int repeatStart = -1, int repeatLength = 0, QString exactCopyValue = {});
    QString design() const { return design_; }
    void setDesign(QString design);
    QString calculatorTheme() const { return theme_; }
    void setCalculatorTheme(QString theme);
    QString calculatorLayout() const { return calculatorLayout_; }
    void setCalculatorLayout(QString layout);
    bool setCustomLayout(const CustomKeypad::Layout &layout, QString *error=nullptr);
    void clearCustomLayout();
    bool hasCustomLayout() const {return customLayout_.has_value();}
    CustomKeypad::Layout customLayout() const {return customLayout_.value_or(CustomKeypad::Layout{});}
    void setComplexEnabled(bool enabled);
    void setComplexResult(bool complex);
    void retranslate(std::function<QString(const char *)> translate);
    void setShiftState(bool shift, bool alpha);
    void setHeldModifiers(bool shift, bool alpha);
    void setRegisterLayer(bool active);
    bool registerLayerActive() const { return registerLayer_; }
    void setLiteralInputBase(int base);
    bool effectiveShift() const { return !registerLayer_ && !effectiveAlpha() && (heldShift_ || shift_); }
    bool effectiveAlpha() const { return !registerLayer_ && (heldAlpha_ || (!heldShift_ && alpha_)); }
    bool hasActiveLayer() const { return effectiveShift() || effectiveAlpha(); }
    void flashToken(QString token);
    bool activateToken(const QString &token);
    void setMemoryActive(bool active);
    void setBusy(bool busy);
    QSize sizeHint() const override { return design_ == "Desktop" ? QSize(430, 600) : QSize(360, 640); }
    QSize minimumSizeHint() const override { return design_ == "Desktop" ? QSize(330, 370) : QSize(320, 480); }
    std::function<void(QString)> onAction;
protected:
    bool eventFilter(QObject *, QEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void changeEvent(QEvent *) override;
private:
    QString design_ = "Desktop", theme_ = "Classic", calculatorLayout_ = "Compact";
    QFrame *screen_ = nullptr;
    QLabel *scientific_ = nullptr, *memoryIndicator_ = nullptr, *approximate_ = nullptr, *status_ = nullptr, *numberKind_ = nullptr;
    QLabel *angleIndicator_ = nullptr, *exactIndicator_ = nullptr, *baseIndicator_ = nullptr, *notationIndicator_ = nullptr;
    MathEditor *editor_ = nullptr, *result_ = nullptr;
    ArgumentGuide *argumentGuide_ = nullptr;
    QComboBox *angle_ = nullptr, *exact_ = nullptr, *base_ = nullptr, *notation_ = nullptr;
    QPushButton *copy_ = nullptr, *settings_ = nullptr;
    QString statusMessage_;
    QList<QPushButton *> memories_;
    QList<CalculatorKey *> desktopKeys_, lcdKeys_, expandedKeys_, customKeys_;
    std::optional<CustomKeypad::Layout> customLayout_;
    bool customWide_=false;
    void rebuildCustomKeys();
    std::function<QString(const char *)> translate_;
    bool memoryActive_ = false, shift_ = false, alpha_ = false, busy_ = false, complexResult_ = false, complexEnabled_ = true;
    bool heldShift_ = false, heldAlpha_ = false, structuredInput_ = true;
    bool keyTooltipsEnabled_ = true, screenTooltipsEnabled_ = true;
    bool refreshingPalette_ = false, arranging_ = false;
    QChar decimalSeparator_ = '.';
    int desktopEffects_ = 0, literalInputBase_ = 0;
    bool registerLayer_ = false;
    void dispatchKeyAction(const QString &token);
    void refreshStatus();
    void arrange();
    void refreshAppearance();
    void refreshKeyTooltips();
    void refreshDefinitions();
    void makeKeys();
};
