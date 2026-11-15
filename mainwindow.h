#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLineEdit;
class QSlider;
class QTableWidget;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void buildInterface();
    void applyTheme(bool dark);
    void addMusicFiles();
    void filterPlaylist(const QString &text);
    void setCurrentSection(const QString &section);

    Ui::MainWindow *ui;
    QTableWidget *playlistTable = nullptr;
    QLineEdit *searchEdit = nullptr;
    QSlider *volumeSlider = nullptr;
    QPushButton *playButton = nullptr;
    bool darkTheme = false;
};

#endif // MAINWINDOW_H
