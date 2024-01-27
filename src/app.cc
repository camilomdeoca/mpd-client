#include "mpd/mpd.hpp"
#include "playlistwin.h"
#include "window.h"
#include "main.h"
#include "equalizer.h"
#include "skin.h"
#include <QApplication>
#include <QSocketNotifier>
#include <QObject>
#include <cstdio>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Theme path argument missing.");
        exit(1);
    }
    
    QApplication app(argc, argv);

    mpd.connect();
    QSocketNotifier *notifier = new QSocketNotifier(mpd.get_fd(), QSocketNotifier::Read);
    QObject::connect(notifier, &QSocketNotifier::activated, []() { mpd.status_change_cb(); });

    skin_load(argv[1]);
    mainwin_create();
    equalizerwin_create();
    playlistwin_create();
    fetch_initial_mpd_data();
    mainwin->show();
    equalizerwin->show();
    playlistwin->show();

    return app.exec();
}
