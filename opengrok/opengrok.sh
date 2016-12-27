#!/bin/sh

# Start the Tomcat server
catalina.sh start && \
while [ true ]; do
    inotifywait --exclude ".git" -e modify -r /opengrok/src
    OPENGROK_TOMCAT_BASE=$CATALINA_HOME /opt/opengrok/bin/OpenGrok index
done
