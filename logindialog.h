#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QComboBox>
#include <QFrame>
#include "mysqlmanager.h"

static const QStringList SECURITY_QUESTIONS = {
    "What was the name of your first pet?",
    "What is your mother's maiden name?",
    "What city were you born in?",
    "What was the name of your first school?",
    "What is your favorite trekking destination?"
};

// Small helper: turns a plain QLineEdit into one with a "Show" toggle
// button next to it, so the user can check what they actually typed.
inline QWidget* makePasswordField(QLineEdit *&editOut)
{
    QWidget *wrapper = new QWidget;
    QHBoxLayout *row = new QHBoxLayout(wrapper);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);

    editOut = new QLineEdit;
    editOut->setEchoMode(QLineEdit::Password);

    QPushButton *toggleBtn = new QPushButton("Show");
    toggleBtn->setFixedWidth(56);
    toggleBtn->setCheckable(true);
    toggleBtn->setStyleSheet(
        "QPushButton { background-color: #33343c; padding: 6px 8px; font-size: 11px; }"
        "QPushButton:checked { background-color: #2f7d6c; }"
        );

    QObject::connect(toggleBtn, &QPushButton::toggled, editOut, [editOut, toggleBtn](bool checked) {
        editOut->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        toggleBtn->setText(checked ? "Hide" : "Show");
    });

    row->addWidget(editOut, 1);
    row->addWidget(toggleBtn);
    return wrapper;
}

// Wipes every widget out of a dialog so the same QDialog can be
// redrawn as a different "step" without opening a second window.
// Used by both ForgotPasswordDialog and LoginDialog to move between
// steps.
//
// NOTE: this deletes every direct child widget of the dialog rather
// than just walking the top-level layout's items. That matters
// because widgets added via a nested QFormLayout (e.g. the Name /
// Email / Password rows) get reparented straight onto the dialog,
// not onto the QFormLayout itself - so only popping items off the
// outer QVBoxLayout misses them, leaving them orphaned but still
// visible on screen (they'd show through underneath the next step).
// Deleting direct-child widgets catches those nested ones too, and
// Qt's parent/child ownership cleans up anything nested inside them
// (like the Show/Hide button inside makePasswordField's wrapper).
inline void clearDialogLayout(QDialog *dlg)
{
    const QList<QWidget*> children =
        dlg->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *w : children)
        delete w;

    delete dlg->layout();
}

// ============================================================
// Forgot password - security question only, no email service
// needed. Simple and fully self-contained.
// ============================================================
class ForgotPasswordDialog : public QDialog
{
public:
    ForgotPasswordDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Reset Password");
        setFixedWidth(380);
        buildEmailStep();
    }

private:
    MySqlManager mysqlManager;
    QString pendingEmail;
    QLineEdit *emailEdit = nullptr;
    QLineEdit *answerEdit = nullptr;
    QLineEdit *newPasswordEdit = nullptr;
    QLineEdit *confirmPasswordEdit = nullptr;
    QLabel *statusLabel = nullptr;

    void buildEmailStep()
    {
        clearDialogLayout(this);
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("Enter the email on your account");
        title->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(title);

        emailEdit = new QLineEdit;
        emailEdit->setPlaceholderText("your.email@example.com");
        layout->addWidget(emailEdit);

        statusLabel = new QLabel("");
        statusLabel->setStyleSheet("color: #e07a5f;");
        statusLabel->setWordWrap(true);
        layout->addWidget(statusLabel);

        QPushButton *nextBtn = new QPushButton("Continue");
        layout->addWidget(nextBtn);

        connect(nextBtn, &QPushButton::clicked, this, [this]() {
            QString email = emailEdit->text().trimmed();
            if (email.isEmpty()) {
                statusLabel->setText("Please enter your email.");
                return;
            }
            QString question = mysqlManager.getSecurityQuestion(email);
            if (question.isEmpty()) {
                statusLabel->setText("No account found with that email.");
                return;
            }
            pendingEmail = email;
            buildAnswerStep(question);
        });
    }

    void buildAnswerStep(QString question)
    {
        clearDialogLayout(this);
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("Answer your security question");
        title->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(title);

        QLabel *questionLabel = new QLabel(question);
        questionLabel->setWordWrap(true);
        questionLabel->setStyleSheet("color: #4fb8a2;");
        layout->addWidget(questionLabel);

        answerEdit = new QLineEdit;
        answerEdit->setPlaceholderText("Your answer");
        layout->addWidget(answerEdit);

        QFrame *divider = new QFrame;
        divider->setFrameShape(QFrame::HLine);
        layout->addWidget(divider);

        layout->addWidget(new QLabel("New password:"));
        layout->addWidget(makePasswordField(newPasswordEdit));

        layout->addWidget(new QLabel("Confirm new password:"));
        layout->addWidget(makePasswordField(confirmPasswordEdit));

        statusLabel = new QLabel("");
        statusLabel->setStyleSheet("color: #e07a5f;");
        statusLabel->setWordWrap(true);
        layout->addWidget(statusLabel);

        QPushButton *resetBtn = new QPushButton("Reset Password");
        layout->addWidget(resetBtn);

        connect(resetBtn, &QPushButton::clicked, this, [this]() {
            if (answerEdit->text().isEmpty() || newPasswordEdit->text().isEmpty()) {
                statusLabel->setText("Fill in your answer and a new password.");
                return;
            }
            if (newPasswordEdit->text() != confirmPasswordEdit->text()) {
                statusLabel->setText("Passwords don't match.");
                return;
            }

            QString errorMessage;
            bool ok = mysqlManager.resetPasswordWithSecurityAnswer(
                pendingEmail, answerEdit->text(), newPasswordEdit->text(), errorMessage);

            if (!ok) {
                statusLabel->setText(errorMessage);
                return;
            }

            QMessageBox::information(this, "Password Reset",
                                     "Your password has been reset. You can now log in.");
            accept();
        });
    }
};

// Shown once when the app starts, before the main window opens.
//
// Sign up is now two steps in the same window:
//   Step 1 (this is the screen you see first): Name / Email / Password,
//           plus Log In, Sign Up, Forgot Password.
//   Step 2 (only reached via "Sign Up"): security question + answer,
//           shown on its own screen so the login screen stays clean.
class LoginDialog : public QDialog
{
public:
    int loggedInUserId = 0;
    QString loggedInUserName = "";
    QString loggedInUserEmail = "";

    LoginDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Nepal Tourism & Trek Planner - Login");
        setFixedWidth(380);
        buildLoginStep();
    }

private:
    // Step 1 widgets
    QLineEdit *nameEdit = nullptr;
    QLineEdit *identifierEdit = nullptr;   // email or username, for login
    QLineEdit *passwordEdit = nullptr;
    QLabel    *statusLabel = nullptr;

    // Step 2 (signup security question) widgets
    QComboBox *securityQuestionCombo = nullptr;
    QLineEdit *securityAnswerEdit = nullptr;
    QLabel    *securityStatusLabel = nullptr;

    // Held between step 1 -> step 2 so we only touch the database
    // once we have everything needed to register.
    QString pendingName;
    QString pendingIdentifier;
    QString pendingPassword;

    MySqlManager mysqlManager;

    void buildLoginStep()
    {
        clearDialogLayout(this);
        setFixedWidth(380);
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("Welcome - log in or create an account");
        title->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(title);

        QFormLayout *form = new QFormLayout;
        nameEdit = new QLineEdit;
        nameEdit->setPlaceholderText("Only needed for sign up");
        identifierEdit = new QLineEdit;
        identifierEdit->setPlaceholderText("Email or username");

        form->addRow("Name (sign up only):", nameEdit);
        form->addRow("Email / Username:", identifierEdit);
        form->addRow("Password:", makePasswordField(passwordEdit));

        layout->addLayout(form);

        statusLabel = new QLabel("");
        statusLabel->setStyleSheet("color: #e07a5f;");
        statusLabel->setWordWrap(true);
        layout->addWidget(statusLabel);

        QPushButton *loginBtn = new QPushButton("Log In");
        QPushButton *signupBtn = new QPushButton("Sign Up");
        QPushButton *forgotBtn = new QPushButton("Forgot Password?");
        forgotBtn->setStyleSheet("background-color: transparent; color: #4fb8a2; text-decoration: underline;");

        layout->addWidget(loginBtn);
        layout->addWidget(signupBtn);
        layout->addWidget(forgotBtn);

        connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::handleLogin);
        connect(signupBtn, &QPushButton::clicked, this, &LoginDialog::goToSecurityStep);
        connect(forgotBtn, &QPushButton::clicked, this, [this]() {
            ForgotPasswordDialog dlg(this);
            dlg.exec();
        });
    }

    // Validates the step-1 fields for signup, stashes them, then
    // swaps the whole dialog body over to the security-question screen.
    void goToSecurityStep()
    {
        if (nameEdit->text().isEmpty() || identifierEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
            statusLabel->setText("Fill in name, email, and password to sign up.");
            return;
        }

        pendingName = nameEdit->text();
        pendingIdentifier = identifierEdit->text();
        pendingPassword = passwordEdit->text();

        buildSecurityStep();
    }

    void buildSecurityStep()
    {
        clearDialogLayout(this);
        // Wider than the login screen (380px) - the question text needs
        // the room, and a side-by-side form label would only squeeze it
        // further, so this step uses a stacked (label-above-field) layout
        // instead of a QFormLayout.
        setFixedWidth(460);
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("Set up account recovery");
        title->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(title);

        QLabel *hint = new QLabel("Pick a security question - it's how you'll recover your password later.");
        hint->setWordWrap(true);
        hint->setStyleSheet("color: #888891; font-size: 11px;");
        layout->addWidget(hint);

        QLabel *questionLabel = new QLabel("Security question:");
        layout->addWidget(questionLabel);

        securityQuestionCombo = new QComboBox;
        securityQuestionCombo->addItems(SECURITY_QUESTIONS);
        securityQuestionCombo->setMinimumWidth(420);
        // The combo shows the currently selected question elided with
        // "..." if it's still too long for the box - so surface the
        // full text as a tooltip too.
        securityQuestionCombo->setToolTip(securityQuestionCombo->currentText());
        connect(securityQuestionCombo, &QComboBox::currentTextChanged,
                securityQuestionCombo, [this](const QString &text) {
                    securityQuestionCombo->setToolTip(text);
                });
        layout->addWidget(securityQuestionCombo);

        QLabel *answerLabel = new QLabel("Answer:");
        layout->addWidget(answerLabel);

        securityAnswerEdit = new QLineEdit;
        securityAnswerEdit->setPlaceholderText("Your answer");
        layout->addWidget(securityAnswerEdit);

        securityStatusLabel = new QLabel("");
        securityStatusLabel->setStyleSheet("color: #e07a5f;");
        securityStatusLabel->setWordWrap(true);
        layout->addWidget(securityStatusLabel);

        QPushButton *createBtn = new QPushButton("Create Account");
        QPushButton *backBtn = new QPushButton("Back");
        backBtn->setStyleSheet("background-color: transparent; color: #4fb8a2; text-decoration: underline;");

        layout->addWidget(createBtn);
        layout->addWidget(backBtn);

        connect(createBtn, &QPushButton::clicked, this, &LoginDialog::handleSignup);
        connect(backBtn, &QPushButton::clicked, this, &LoginDialog::buildLoginStep);
    }

    void handleLogin()
    {
        if (identifierEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
            statusLabel->setText("Enter your email/username and password.");
            return;
        }

        UserRow user = mysqlManager.authenticateUser(identifierEdit->text(), passwordEdit->text());

        if (user.userId == -1) {
            statusLabel->setText("Login failed. Check your details, or use Forgot Password.");
            return;
        }

        loggedInUserId = user.userId;
        loggedInUserName = QString::fromStdString(user.name);
        loggedInUserEmail = identifierEdit->text().contains("@") ? identifierEdit->text() : "";
        accept();
    }

    void handleSignup()
    {
        if (securityAnswerEdit->text().isEmpty()) {
            securityStatusLabel->setText("Answer a security question so you can recover your account later.");
            return;
        }

        QString errorMessage;
        int newUserId = mysqlManager.registerUser(
            pendingName, pendingIdentifier, pendingPassword,
            securityQuestionCombo->currentText(), securityAnswerEdit->text(),
            errorMessage);

        if (newUserId == -1) {
            securityStatusLabel->setText("Sign up failed: " + errorMessage);
            return;
        }

        loggedInUserId = newUserId;
        loggedInUserName = pendingName;
        loggedInUserEmail = pendingIdentifier;
        QMessageBox::information(this, "Account Created", "Welcome, " + loggedInUserName + "!");
        accept();
    }
};

#endif // LOGINDIALOG_H
