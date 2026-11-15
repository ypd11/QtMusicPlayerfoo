#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QAudioOutput;
class QLabel;
class QLineEdit;
class QMediaPlayer;
class QPushButton;
class QSlider;
class QTableWidget;

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

    void playSelectedTrack();
    void playTrack(int row);
    void playNextTrack();
    void playPreviousTrack();
    void stopPlayback();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    QString formatTime(qint64 milliseconds) const;

    Ui::MainWindow *ui;
    QMediaPlayer *player = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QTableWidget *playlistTable = nullptr;
    QLineEdit *searchEdit = nullptr;
    QSlider *volumeSlider = nullptr;
    QSlider *seekSlider = nullptr;
    QLabel *positionLabel = nullptr;
    QLabel *durationLabel = nullptr;
    QPushButton *playButton = nullptr;
    bool darkTheme = false;
    bool seeking = false;
    int currentTrackRow = -1;
};

#endif // MAINWINDOW_H
