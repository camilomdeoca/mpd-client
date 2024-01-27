find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

qt_add_executable(mpd-client
    app.cc
    skin.cc
    skins_util.cc
    skins_cfg.cc
    window.cc
    main.cc
    widget.cc
    button.cc
    hslider.cc
    vis.cc
    svis.cc
    textbox.cc
    dock.cc
    eq-slider.cc
    eq-graph.cc
    equalizer.cc
    settings.cc
    playlist-widget.cc
    playlist-slider.cc
    playlistwin.cc
    util.cc
    drag-handle.cc
    mpd/mpd.cpp
    mpd/song.cpp
    mpd/status.cpp
    mpd/playlist.cpp
    number.cc
    menurow.cc
    monostereo.cc
    playstatus.cc
    inifile.cc
)

target_link_libraries(mpd-client PRIVATE Qt6::Widgets glib-2.0 mpdclient)
