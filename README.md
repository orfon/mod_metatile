**ALPHA quality software**

This is my first apache module so do not use unless you know more than I do.

## mod_metatile

If you are generating metatiles and want to simply serve them as tiles with apache, this might be the module for you. Unlike the famous [mod_tile](https://github.com/openstreetmap/mod_tile) this module is much simpler and only *delivers* tiles from metatiles. There is no logic to generate tiles.

Basically, I stripped the metatile-reading code from mod_tile and wrapped it in a simple module.

## apache config example

build:

    apxs -i -a -c mod_metatile.c metatile.c

load:

    LoadModule metatile_module    /usr/lib/apache2/modules/mod_metatile.so

MetatilePath can be configured per <Location>, <Directory> or <Vhost>:

    LogLevel metatile:debug
    <Location /meta-test/>
        AddHandler metatile-handler .png .json
        MetatileEtag On
        MetatilePath /root-path-to-meta/
    </Location>
