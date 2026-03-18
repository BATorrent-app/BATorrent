#ifndef CREATETORRENTDIALOG_H
#define CREATETORRENTDIALOG_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QTextEdit;
class QProgressBar;

class CreateTorrentDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateTorrentDialog(QWidget *parent = nullptr);

    QString sourcePath() const;
    QString outputPath() const;
    QString trackerUrl() const;
    QString comment() const;
    int pieceSize() const;

private slots:
    void browseSource();
    void browseOutput();
    void createTorrent();

private:
    QLineEdit *m_sourceEdit;
    QLineEdit *m_outputEdit;
    QTextEdit *m_trackerEdit;
    QLineEdit *m_commentEdit;
    QSpinBox *m_pieceSizeSpin;
    QProgressBar *m_progressBar;
};

#endif
