# Express Updater

Express Updater is a simple tool that was designed to help software developers to easily provide the latest versions of their desktop applications (Windows, Linux, Mac) to users.

ExpressUpdater is an automatic app updater/installer for desktop apps. Although now working only for Windows, it should be easily portable
to 3 main desktop platforms (Windows, Linux, Mac). It consists out of two components, web service in node.js which serves the latest version
of the app, and desktop launcher in written C++ using CEF (https://bitbucket.org/chromiumembedded/cef/src/master/). Launcher updates the
app by contacting the web service, downloading only the files which had been changed, and launches the app afterwards.

![Screenshot](https://bytebucket.org/Nemandza/expressupdater/raw/32812b6f7b7d1b5808233d8ee0af691eb2b2f416/docs/screenshot.png)

# Setup

## Launcher

### Windows

 * VS 2017 is required (Community edition is enough).
 * Just checkout the code, open the solution and you should be able to compile and run the updater.
 * In the file [render_process.cpp](https://bitbucket.org/Nemandza/expressupdater/src/master/src/explauncher/render_process.cpp) you can find all the constants which you need to edit
to set up the launcher for your app. Like the URL of the web service, destination folder for the app,
binary name and command line parameters, and so on.

### Linux/Mac

 * Linux/Mac support is still work in progress. 
 * A build system should be set up (eg Makefiles, CMake).
 * Parts of the code should be ported. Eg starting the updated app in another process.

## Service

 * Requirement is [node.js](https://nodejs.org/en/) with [express.js](https://expressjs.com/) module.
     * If new to node check out the following tutorials: [Node.js Tutorial for Beginners](https://www.youtube.com/watch?v=TlB_eWDSMt4&t=1s),
       [Express.js Tutorial](https://www.youtube.com/watch?v=pKd0Rpw7O48&t=2163s).
 * You should place your app binaries in the sub-folder of the folder where expressapp.js is placed. For 
Windows folder should be named "appwin" (although that can be changed).
 * Running [expressapp.js](https://bitbucket.org/Nemandza/expressupdater/src/master/expserver/expressapp.js) with node (node expressapp.js) should start the web service for serving your app. 

# License

ExpressUpdater is licensed under [MIT license](LICENSE.md).