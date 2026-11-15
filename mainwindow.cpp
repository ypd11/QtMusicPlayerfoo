#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    buildInterface();

    QSettings settings;
    darkTheme = settings.value("appearance/darkTheme", false).toBool();
    applyTheme(darkTheme);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buildInterface()
{
    setWindowTitle("QtMusicPlayerfoo");
    resize(1120, 720);
    setMinimumSize(820, 540);

    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *addFilesAction = fileMenu->addAction("Add music files...");
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::addMusicFiles);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *playbackMenu = menuBar()->addMenu("&Playback");
    playbackMenu->addAction("Play / Pause", this, [this]() {
        if (playButton)
            playButton->click();
    });
    playbackMenu->addAction("Stop", this, [this]() {
        if (playButton) {
            playButton->setText("Play");
            statusBar()->showMessage("Playback engine will be added next", 3000);
        }
    });

    QMenu *viewMenu = menuBar()->addMenu("&View");
    QAction *lightAction = viewMenu->addAction("Light theme");
    QAction *darkAction = viewMenu->addAction("Dark theme");
    lightAction->setCheckable(true);
    darkAction->setCheckable(true);
    connect(lightAction, &QAction::triggered, this, [this]() { applyTheme(false); });
    connect(darkAction, &QAction::triggered, this, [this]() { applyTheme(true); });

    QWidget *root = new QWidget(this);
    root->setObjectName("root");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(7);

    // Compact top toolbar
    auto *toolbar = new QWidget(root);
    toolbar->setObjectName("toolbar");
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(7, 5, 7, 5);
    toolbarLayout->setSpacing(6);

    auto *addButton = new QPushButton("Add files", toolbar);
    addButton->setObjectName("toolButton");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addMusicFiles);
    toolbarLayout->addWidget(addButton);

    auto *addFolderButton = new QPushButton("Add folder", toolbar);
    addFolderButton->setObjectName("toolButton");
    connect(addFolderButton, &QPushButton::clicked, this, [this]() {
        const QString folder = QFileDialog::getExistingDirectory(this, "Choose music folder");
        if (!folder.isEmpty()) {
            statusBar()->showMessage("Folder scanning will be added next: " + folder, 4000);
        }
    });
    toolbarLayout->addWidget(addFolderButton);
    toolbarLayout->addSpacing(12);

    auto *toolbarTitle = new QLabel("PLAYLIST", toolbar);
    toolbarTitle->setObjectName("toolbarTitle");
    toolbarLayout->addWidget(toolbarTitle);
    toolbarLayout->addStretch();

    searchEdit = new QLineEdit(toolbar);
    searchEdit->setObjectName("searchEdit");
    searchEdit->setPlaceholderText("Search playlist...");
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setMaximumWidth(250);
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::filterPlaylist);
    toolbarLayout->addWidget(searchEdit);
    rootLayout->addWidget(toolbar);

    // Main workspace: navigation on the left, playlist on the right
    auto *workspace = new QWidget(root);
    auto *workspaceLayout = new QHBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(7);

    auto *sidebar = new QWidget(workspace);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(178);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(7, 10, 7, 7);
    sidebarLayout->setSpacing(3);

    auto addSectionLabel = [sidebarLayout, sidebar](const QString &text) {
        auto *label = new QLabel(text, sidebar);
        label->setObjectName("sectionLabel");
        sidebarLayout->addWidget(label);
    };
    auto addNavButton = [this, sidebarLayout, sidebar](const QString &text) {
        auto *button = new QPushButton(text, sidebar);
        button->setObjectName("navButton");
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(29);
        sidebarLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, text]() {
            setCurrentSection(text);
        });
        return button;
    };

    addSectionLabel("LIBRARY");
    addNavButton("Music");
    addNavButton("Recently added");
    addNavButton("Most played");
    sidebarLayout->addSpacing(10);
    addSectionLabel("PLAYLISTS");
    addNavButton("All tracks");
    addNavButton("Favorites");
    sidebarLayout->addSpacing(10);
    addSectionLabel("RADIO");
    addNavButton("Radio stations");
    sidebarLayout->addStretch();
    auto *sidebarHint = new QLabel("Local collection", sidebar);
    sidebarHint->setObjectName("mutedLabel");
    sidebarLayout->addWidget(sidebarHint);

    auto *playlistPanel = new QWidget(workspace);
    playlistPanel->setObjectName("playlistPanel");
    auto *playlistLayout = new QVBoxLayout(playlistPanel);
    playlistLayout->setContentsMargins(9, 8, 9, 9);
    playlistLayout->setSpacing(7);

    auto *playlistHeading = new QHBoxLayout();
    auto *playlistName = new QLabel("Default playlist", playlistPanel);
    playlistName->setObjectName("panelTitle");
    playlistHeading->addWidget(playlistName);
    playlistHeading->addStretch();
    auto *trackCount = new QLabel("0 tracks", playlistPanel);
    trackCount->setObjectName("mutedLabel");
    playlistHeading->addWidget(trackCount);
    playlistLayout->addLayout(playlistHeading);

    playlistTable = new QTableWidget(0, 5, playlistPanel);
    playlistTable->setObjectName("playlistTable");
    playlistTable->setHorizontalHeaderLabels({"#", "Title", "Artist", "Album", "Length"});
    playlistTable->verticalHeader()->setVisible(false);
    playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistTable->setShowGrid(false);
    playlistTable->setAlternatingRowColors(true);
    playlistTable->setSortingEnabled(true);
    playlistTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    playlistTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    playlistTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    playlistTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    playlistTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    playlistLayout->addWidget(playlistTable);

    workspaceLayout->addWidget(sidebar);
    workspaceLayout->addWidget(playlistPanel, 1);
    rootLayout->addWidget(workspace, 1);

    // Playback strip
    auto *playerBar = new QWidget(root);
    playerBar->setObjectName("playerBar");
    auto *playerLayout = new QHBoxLayout(playerBar);
    playerLayout->setContentsMargins(9, 7, 9, 7);
    playerLayout->setSpacing(7);

    auto *previousButton = new QPushButton("|<", playerBar);
    previousButton->setObjectName("transportButton");
    previousButton->setToolTip("Previous track");
    connect(previousButton, &QPushButton::clicked, this, [this]() {
        statusBar()->showMessage("Playback engine will be added next", 2500);
    });
    playerLayout->addWidget(previousButton);

    playButton = new QPushButton("Play", playerBar);
    playButton->setObjectName("playButton");
    playButton->setMinimumWidth(64);
    connect(playButton, &QPushButton::clicked, this, [this]() {
        const bool playing = playButton->text() == "Play";
        playButton->setText(playing ? "Pause" : "Play");
        statusBar()->showMessage("Playback engine will be added next", 2500);
    });
    playerLayout->addWidget(playButton);

    auto *nextButton = new QPushButton(">|", playerBar);
    nextButton->setObjectName("transportButton");
    nextButton->setToolTip("Next track");
    connect(nextButton, &QPushButton::clicked, this, [this]() {
        statusBar()->showMessage("Playback engine will be added next", 2500);
    });
    playerLayout->addWidget(nextButton);

    auto *stopButton = new QPushButton("Stop", playerBar);
    stopButton->setObjectName("transportButton");
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        playButton->setText("Play");
        statusBar()->showMessage("Stopped", 2000);
    });
    playerLayout->addWidget(stopButton);

    auto *timeLabel = new QLabel("00:00", playerBar);
    timeLabel->setObjectName("mutedLabel");
    playerLayout->addWidget(timeLabel);

    auto *seekSlider = new QSlider(Qt::Horizontal, playerBar);
    seekSlider->setObjectName("seekSlider");
    seekSlider->setRange(0, 1000);
    seekSlider->setValue(0);
    seekSlider->setEnabled(false);
    playerLayout->addWidget(seekSlider, 1);

    auto *durationLabel = new QLabel("--:--", playerBar);
    durationLabel->setObjectName("mutedLabel");
    playerLayout->addWidget(durationLabel);
    playerLayout->addSpacing(12);

    auto *volumeLabel = new QLabel("Vol", playerBar);
    volumeLabel->setObjectName("mutedLabel");
    playerLayout->addWidget(volumeLabel);
    volumeSlider = new QSlider(Qt::Horizontal, playerBar);
    volumeSlider->setObjectName("volumeSlider");
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(70);
    volumeSlider->setFixedWidth(95);
    playerLayout->addWidget(volumeSlider);

    rootLayout->addWidget(playerBar);
    setCentralWidget(root);
    statusBar()->showMessage("Ready", 2000);
}

void MainWindow::applyTheme(bool dark)
{
    darkTheme = dark;
    QSettings settings;
    settings.setValue("appearance/darkTheme", darkTheme);

    const QString lightStyle = R"(
        QMainWindow, QWidget#root { background: #e9e9e9; color: #202020; }
        QMenuBar { background: #e5e5e5; color: #202020; border-bottom: 1px solid #c7c7c7; }
        QMenuBar::item { padding: 4px 9px; background: transparent; }
        QMenuBar::item:selected, QMenu::item:selected { background: #d5e5f5; }
        QMenu { background: #f4f4f4; color: #202020; border: 1px solid #bdbdbd; }
        QMenu::item { padding: 5px 24px 5px 20px; }
        QWidget#toolbar, QWidget#playerBar { background: #dedede; border: 1px solid #c5c5c5; }
        QWidget#sidebar { background: #e3e3e3; border: 1px solid #c5c5c5; }
        QWidget#playlistPanel { background: #f2f2f2; border: 1px solid #c5c5c5; }
        QLabel#sectionLabel { color: #666666; font-size: 10px; font-weight: bold; padding: 7px 5px 3px; }
        QLabel#toolbarTitle { color: #555555; font-size: 10px; font-weight: bold; }
        QLabel#panelTitle { color: #202020; font-weight: bold; }
        QLabel#mutedLabel { color: #777777; }
        QPushButton { color: #202020; }
        QPushButton#toolButton, QPushButton#transportButton { background: #eeeeee; border: 1px solid #bdbdbd; padding: 4px 10px; }
        QPushButton#toolButton:hover, QPushButton#transportButton:hover { background: #ffffff; border-color: #8e8e8e; }
        QPushButton#playButton { background: #d8e8f7; border: 1px solid #8caac5; padding: 5px 12px; font-weight: bold; }
        QPushButton#playButton:hover { background: #c8def1; }
        QPushButton#navButton { text-align: left; background: transparent; border: 1px solid transparent; padding: 4px 8px; }
        QPushButton#navButton:hover { background: #d8d8d8; border-color: #c5c5c5; }
        QLineEdit#searchEdit { background: #ffffff; color: #202020; border: 1px solid #bdbdbd; padding: 5px 7px; }
        QTableWidget#playlistTable { background: #fafafa; alternate-background-color: #f0f0f0; color: #202020; border: 1px solid #c8c8c8; selection-background-color: #c9dced; selection-color: #17212b; }
        QHeaderView::section { background: #e5e5e5; color: #333333; border: none; border-right: 1px solid #c9c9c9; border-bottom: 1px solid #c9c9c9; padding: 5px 7px; }
        QSlider::groove:horizontal { height: 4px; background: #bdbdbd; }
        QSlider::handle:horizontal { background: #777777; width: 10px; margin: -4px 0; }
        QStatusBar { background: #e5e5e5; color: #555555; border-top: 1px solid #c7c7c7; }
    )";

    const QString darkStyle = R"(
        QMainWindow, QWidget#root { background: #202124; color: #dedede; }
        QMenuBar { background: #252629; color: #dedede; border-bottom: 1px solid #393b40; }
        QMenuBar::item { padding: 4px 9px; background: transparent; }
        QMenuBar::item:selected, QMenu::item:selected { background: #343a42; }
        QMenu { background: #292b30; color: #dedede; border: 1px solid #45484f; }
        QMenu::item { padding: 5px 24px 5px 20px; }
        QWidget#toolbar, QWidget#playerBar { background: #292b30; border: 1px solid #3d4046; }
        QWidget#sidebar { background: #25272b; border: 1px solid #3d4046; }
        QWidget#playlistPanel { background: #292b30; border: 1px solid #3d4046; }
        QLabel#sectionLabel { color: #92969e; font-size: 10px; font-weight: bold; padding: 7px 5px 3px; }
        QLabel#toolbarTitle { color: #a5a8af; font-size: 10px; font-weight: bold; }
        QLabel#panelTitle { color: #eeeeee; font-weight: bold; }
        QLabel#mutedLabel { color: #92969e; }
        QPushButton { color: #dedede; }
        QPushButton#toolButton, QPushButton#transportButton { background: #33363c; border: 1px solid #4a4e56; padding: 4px 10px; }
        QPushButton#toolButton:hover, QPushButton#transportButton:hover { background: #3c4047; border-color: #626873; }
        QPushButton#playButton { background: #344b43; border: 1px solid #527d6c; padding: 5px 12px; font-weight: bold; }
        QPushButton#playButton:hover { background: #3c5a4f; }
        QPushButton#navButton { text-align: left; background: transparent; border: 1px solid transparent; padding: 4px 8px; }
        QPushButton#navButton:hover { background: #34373d; border-color: #454951; }
        QLineEdit#searchEdit { background: #202226; color: #eeeeee; border: 1px solid #484c54; padding: 5px 7px; }
        QTableWidget#playlistTable { background: #222428; alternate-background-color: #292c31; color: #dedede; border: 1px solid #41454c; selection-background-color: #3b4c5c; selection-color: #ffffff; }
        QHeaderView::section { background: #303238; color: #c7c9ce; border: none; border-right: 1px solid #41454c; border-bottom: 1px solid #41454c; padding: 5px 7px; }
        QSlider::groove:horizontal { height: 4px; background: #4a4e55; }
        QSlider::handle:horizontal { background: #aeb4bd; width: 10px; margin: -4px 0; }
        QStatusBar { background: #25272b; color: #a5a8af; border-top: 1px solid #3d4046; }
    )";

    setStyleSheet(dark ? darkStyle : lightStyle);
}

void MainWindow::addMusicFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Add music files", QString(),
        "Audio files (*.mp3 *.flac *.wav *.ogg *.opus *.m4a *.aac *.wma);;All files (*.*)");

    for (const QString &file : files) {
        const int row = playlistTable->rowCount();
        playlistTable->insertRow(row);
        const QFileInfo info(file);
        playlistTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        playlistTable->setItem(row, 1, new QTableWidgetItem(info.completeBaseName()));
        playlistTable->setItem(row, 2, new QTableWidgetItem(""));
        playlistTable->setItem(row, 3, new QTableWidgetItem(""));
        playlistTable->setItem(row, 4, new QTableWidgetItem("--:--"));
        playlistTable->item(row, 1)->setData(Qt::UserRole, file);
    }

    statusBar()->showMessage(QString("%1 file(s) added to the playlist").arg(files.size()), 3000);
}

void MainWindow::filterPlaylist(const QString &text)
{
    const QString query = text.trimmed();
    for (int row = 0; row < playlistTable->rowCount(); ++row) {
        bool matches = query.isEmpty();
        for (int column = 1; column < playlistTable->columnCount() && !matches; ++column) {
            QTableWidgetItem *item = playlistTable->item(row, column);
            matches = item && item->text().contains(query, Qt::CaseInsensitive);
        }
        playlistTable->setRowHidden(row, !matches);
    }
}

void MainWindow::setCurrentSection(const QString &section)
{
    statusBar()->showMessage(section + " selected", 2500);
}
