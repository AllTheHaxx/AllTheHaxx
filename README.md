##AllTheHaxx
###The better DDNet Client ~ VERKECKT! [![Circle CI](https://circleci.com/gh/Henningstone/AllTheHaxx/tree/master.svg?style=svg)](https://circleci.com/gh/Henningstone/AllTheHaxx/tree/master)
================================
Our own flavor of [DDNet](https://github.com/ddnet/ddnet) Client. See the [website](http:/henningstone.github.io) for more information.

Development discussions happen on #AllTheHaxx on Quakenet ([Webchat](http://webchat.quakenet.org/?channels=AllTheHaxx&uio=d4)).

You can get binary releases from [Bintray](https://dl.bintray.com/henningstone/tw_AllTheHaxx/).

Building
--------

To compile AllTheHaxx yourself, you can follow the [instructions for compiling Teeworlds](https://www.teeworlds.com/?page=docs&wiki=compiling_everything).
Make sure you have all the requirements that are needed by DDNet Client (listed below)

--------------------------
DDNet requires additional libraries, that are bundled for the most common platforms (Windows, Mac, Linux, all x86 and x86_64). Instead you can install these libraries on your system, remove the `config.lua` and `bam` should use the system-wide libraries by default. You can install all required dependencies on Debian and Ubuntu like this:

    apt-get install libsdl1.2-dev libfreetype6-dev libcurl4-openssl-dev libogg-dev libopus-dev libopusfile-dev

If you have the libraries installed, but still want to use the bundled ones instead, you can specify so by running `bam config curl.use_pkgconfig=false opus.use_pkgconfig=false opusfile.use_pkgconfig=false ogg.use_pkgconfig=false`.

The MySQL server is not included in the binary releases and can be built with `bam server_sql_release`. It requires `libmariadbclient-dev`, `libmysqlcppconn-dev` and `libboost-dev`, which are also bundled for the common platforms.

--------------------------
