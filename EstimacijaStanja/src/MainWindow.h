#pragma once
#include <gui/Window.h>
#include "MainView.h"

class MainWindow : public gui::Window
{
protected:
    MainView _mainView;

public:
    MainWindow()
    : gui::Window(gui::Size(900, 700))
    {
        setTitle("PMU estimacija stanja");
        setCentralView(&_mainView);
    }

    bool shouldClose() override { return true; }
};
