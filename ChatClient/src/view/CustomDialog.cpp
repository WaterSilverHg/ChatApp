#include "CustomDialog.h"
#include <QPushButton>
#include <QStyle>
#include <QApplication>

static const QString MSGBOX_STYLE = QStringLiteral(
    "QMessageBox { background-color: #FFFFFF; }"
    "QLabel { color: #333; font-size: 13px; }"
    "QPushButton { background-color: #A8D8B9; color: #333; border: none; border-radius: 5px; "
    "  padding: 8px 20px; font-size: 13px; font-weight: bold; min-width: 80px; }"
    "QPushButton:hover { background-color: #7CB894; color: white; }");

static void applyIcon(QWidget* w) {
    w->setWindowIcon(QIcon(":/chat.svg"));
}

CustomDialog::CustomDialog(QWidget* parent)
    : QDialog(parent)
{
    setupStyle();
    applyIcon(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

CustomDialog::CustomDialog(const QString& title, QWidget* parent)
    : QDialog(parent)
{
    setupStyle();
    applyIcon(this);
    setWindowTitle(title);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

CustomDialog::~CustomDialog()
{}

void CustomDialog::setupStyle()
{
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #FFFFFF; }"
        "QLabel { color: #333; font-size: 13px; }"
        "QPushButton { background-color: #A8D8B9; color: #333; border: none; border-radius: 5px; "
        "  padding: 8px 16px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #7CB894; color: white; }"
        "QLineEdit { background: #F5FAF7; border: 1px solid #A8D8B9; border-radius: 4px; "
        "  padding: 6px 10px; font-size: 13px; }"
        "QTextEdit { background: #F5FAF7; border: 1px solid #A8D8B9; border-radius: 4px; }"
        "QListWidget { background: white; border: 1px solid #A8D8B9; border-radius: 4px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #e0e8e3; }"
        "QListWidget::item:selected { background-color: #E8F5ED; }"
        "QTabWidget::pane { border: 1px solid #A8D8B9; }"
        "QTabBar::tab { background: #A8D8B9; color: #333; padding: 4px 12px; }"
        "QTabBar::tab:selected { background: #7CB894; color: white; }"
    ));
}

void CustomDialog::information(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
    msgBox.setStyleSheet(MSGBOX_STYLE);
    applyIcon(&msgBox);
    msgBox.exec();
}

void CustomDialog::warning(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
    msgBox.setStyleSheet(MSGBOX_STYLE);
    applyIcon(&msgBox);
    msgBox.exec();
}

void CustomDialog::critical(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox msgBox(QMessageBox::Critical, title, text, QMessageBox::Ok, parent);
    msgBox.setStyleSheet(MSGBOX_STYLE);
    applyIcon(&msgBox);
    msgBox.exec();
}

bool CustomDialog::question(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox msgBox(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::No, parent);
    msgBox.setStyleSheet(MSGBOX_STYLE);
    applyIcon(&msgBox);
    return msgBox.exec() == QMessageBox::Yes;
}

StyledMessageBox::StyledMessageBox(QWidget* parent)
    : QMessageBox(parent)
{
    setStyleSheet(MSGBOX_STYLE);
    applyIcon(this);
}

StyledMessageBox::StyledMessageBox(Icon icon, const QString& title, const QString& text,
                                   StandardButtons buttons, QWidget* parent)
    : QMessageBox(icon, title, text, buttons, parent)
{
    setStyleSheet(MSGBOX_STYLE);
    applyIcon(this);
}
