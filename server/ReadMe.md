Run make to create the webserver

make any cgi files executable

make my-histogram.cgi executable

make python youtube link processing code executable

To use my-histogram functionality, input dir=directory in the request header

    e.g. my-histogram.cgi?dir=./

modules to install for youtube link processing python file:

    pip install youtube_transcript_api
    sudo apt-get install libcurl4-gnutls-dev

Control + C to shutdown the webserver

Cache is implemented with first-in-first-out logic
    
    toggle using command-line args
    for cache: -c cachesize

You can set the ip address using the option -p IPADDRESS; it defaults to localhost/loopback ip address 127.0.0.1
