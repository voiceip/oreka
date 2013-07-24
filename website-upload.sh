#!/bin/bash
rsync --exclude=.svn --archive --progress --links --hard-links website/ henrih@web.sourceforge.net:/home/project-web/oreka/htdocs
