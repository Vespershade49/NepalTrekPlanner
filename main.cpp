#include "mainwindow.h"
#include "logindialog.h"
#include <QApplication>

// One dark, modern theme applied to every window in the app -
// login screen and main window both pick this up automatically.
static const char* APP_STYLE = R"(
    QWidget {
        background-color: #1e1f24;
        color: #e8e8e8;
        font-size: 13px;
    }
    QMainWindow, QDialog {
        background-color: #1e1f24;
    }
    QMenuBar {
        background-color: #23242b;
        color: #e8e8e8;
        padding: 4px;
    }
    QMenuBar::item:selected {
        background-color: #2f7d6c;
        border-radius: 4px;
    }
    QMenu {
        background-color: #26272e;
        border: 1px solid #3a3b42;
    }
    QMenu::item:selected {
        background-color: #2f7d6c;
    }
    /* ---- Tabs ---- */
    QTabWidget::pane {
        border: 1px solid #3a3b42;
        border-radius: 6px;
        top: -1px;
    }
    QTabBar::tab {
        background: #26272e;
        color: #a8a8b0;
        padding: 10px 18px;
        margin-right: 2px;
        border-top-left-radius: 6px;
        border-top-right-radius: 6px;
        font-weight: 500;
    }
    QTabBar::tab:selected {
        background: #2f7d6c;
        color: #ffffff;
        font-weight: bold;
    }
    QTabBar::tab:hover:!selected {
        background: #33343c;
    }
    /* ---- Buttons ---- */
    QPushButton {
        background-color: #2f7d6c;
        color: #ffffff;
        border: none;
        border-radius: 6px;
        padding: 9px 16px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #37937f;
    }
    QPushButton:pressed {
        background-color: #25655a;
    }
    QPushButton:disabled {
        background-color: #3a3b42;
        color: #77787f;
    }
    /* ---- Inputs ---- */
    QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QTextEdit {
        background-color: #26272e;
        border: 1px solid #3a3b42;
        border-radius: 5px;
        padding: 7px 10px;
        color: #e8e8e8;
        min-height: 22px;
    }
    QComboBox {
        min-width: 160px;
    }
    QSpinBox, QDoubleSpinBox {
        min-width: 90px;
    }
    QDateEdit {
        min-width: 120px;
    }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus,
    QDoubleSpinBox:focus, QDateEdit:focus, QTextEdit:focus {
        border: 1px solid #2f7d6c;
    }
    QComboBox::drop-down {
        border: none;
        width: 24px;
    }
    /* ---- Tables ---- */
    QTableWidget {
        background-color: #202126;
        alternate-background-color: #26272e;
        gridline-color: #3a3b42;
        border: 1px solid #3a3b42;
        border-radius: 6px;
    }
    QHeaderView::section {
        background-color: #26272e;
        color: #cfcfd6;
        padding: 8px;
        border: none;
        border-bottom: 2px solid #2f7d6c;
        font-weight: bold;
    }
    QTableWidget::item:selected {
        background-color: #2f7d6c;
        color: #ffffff;
    }
    /* ---- Group boxes ---- */
    QGroupBox {
        border: 1px solid #3a3b42;
        border-radius: 8px;
        margin-top: 12px;
        padding-top: 14px;
        font-weight: bold;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 6px;
        color: #4fb8a2;
    }
    /* ---- Scrollbars ---- */
    QScrollBar:vertical {
        background: #1e1f24;
        width: 12px;
    }
    QScrollBar::handle:vertical {
        background: #3a3b42;
        border-radius: 6px;
        min-height: 24px;
    }
    QScrollBar::handle:vertical:hover {
        background: #4a4b54;
    }
)";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(APP_STYLE);

    // Loop so "Log Out" from inside the app just drops the user back
    // to the login screen instead of closing the whole application.
    while (true) {
        LoginDialog login;
        if (login.exec() != QDialog::Accepted)
            return 0; // user closed the login window - exit for real

        MainWindow *w = new MainWindow(login.loggedInUserId, login.loggedInUserName, login.loggedInUserEmail);

        bool loggedOut = false;
        QObject::connect(w, &MainWindow::logoutRequested, [&loggedOut]() {
            loggedOut = true;
        });

        w->show();
        a.exec();
        delete w;

        if (!loggedOut)
            return 0; // window was closed normally, not via Log Out
    }
}
