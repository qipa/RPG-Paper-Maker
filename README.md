# RPG Paper Maker
--------------

[![Join the chat at https://gitter.im/RPG-Paper-Maker/Lobby](https://badges.gitter.im/RPG-Paper-Maker/Lobby.svg)](https://gitter.im/RPG-Paper-Maker/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build](https://travis-ci.org/Wano-k/RPG-Paper-Maker.svg?branch=develop)](https://travis-ci.org/Wano-k/RPG-Paper-Maker)
[![Build](https://ci.appveyor.com/api/projects/status/iee74loc36pu1r0t?svg=true)](https://ci.appveyor.com/project/Wano-k/rpg-paper-maker)

RPG Paper Maker is a free game maker engine including a 3D universe with many 2D sprites and some simplified 3D objects. Pretty close to the RPG Maker series, RPG Paper Maker finally give a way to create RPGs in a totally 3D universe as easy as it is in 2D!

## How to build the project

* Install Qt on your OS with 
[this installer](https://www.qt.io/download-open-source/#section-2). During the installation, you should select the most recent Qt version, and Qt creator IDE. You should also install gcc if it is not already done. For windows users, the operation is more painful and you will need to install MinGW.
* Run QtCreator.
* Open the engine.pro or game.pro file.
* Run qmake (right click on project root).
* Build and run the project.

If you are having any error that means that you are missing a package. Check the error and try to find out what's missing. Please report any kind of error at Wanok.rpm@gmail.com to help other contributors.

## Contribute to the project

You can help by contributing on the engine or/and the game engine. First, be sure to be familiar with **git**, how to **fork a project** and how to **submit a pull request**.

That means that you should:

* Click on the fork button.
* Clone your fork.
* Add a remote to the original project in order to always have access to the `develop` branch:

#

    git remote add rpm https://github.com/Wano-k/RPG-Paper-Maker.git

* Before starting any correction, you should first pull the develop progress in order to be sure that you have the most recent version of development:

#

    git pull rpm develop

After that, you should create a branch for your correction:
    
    git checkout -b <choose_a_branch_name>

When your correction is done, push it (always in your branch). Finally, just propose you branch as a new pull request in the original project.

### How to contribute
RPG Paper Maker has a `master` branch for stable releases and a `develop` branch for development where you should always try to submit your features.

**If you want to add new features or correct a bug** for the next stable version, we are always using this [BACKLOG](https://docs.google.com/spreadsheets/d/1_gKiMl5pXQjj8QwfN6QEIDqjRJ4e77AgtEafzPKTzQs/edit?usp=sharing) which is listing all the tasks that needs to be done according to priority and difficulty. If you would like to do one of the task in the backlog, you should signal that you are on it by creating an issue in this git. Same thing if you want to do anything that is not already in the backlog. See also the [TODO list](https://github.com/Wano-k/RPG-Paper-Maker/projects/1) for accessible features addings.

Try to respect the code style as much as possible. **We are much more strict on the game scripts' code** that is a code that can be edited by the user and particularly needs to be clear.

### Documentation
Always check out our [documentation](http://rpg-paper-maker.com/index.php/documentation) here (not completed yet).

## Project organization
There are two main systems:

* Engine
* Game

`Engine` is the game editor software (RPG Paper Maker). It is entirely written in `C++` and is managing all the `json` files linked to a project such as maps, statistics...

`Game` is the game engine. We are using the executable and scripts in order to create a fresh new project in the software. This program is using `QML` and `Javascript`. It is also interpreting all the `json` files that are generated by the game editor software.

## License

RPG Paper Maker is a free software under GNU GPLv.3 license. See LICENSE.txt.
