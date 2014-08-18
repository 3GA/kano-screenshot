#!/bin/bash
#
# Quick & easy tests for kano-screenshot use cases
#

sc=./kano-screenshot

# width tests
`$sc -w 320 -h 200 -p test1.png`
`$sc -w 320 -p test2.png`

# height tests
`$sc -h 200 -w 200 -p test3.png`
`$sc -h 320 -p test4.png`

# fullscreen
`$sc -p test5.png`

# cropping
`$sc -c 10,10,175,300 -p test6.png`

# application mode
zenity --info "kano screenshot test application" --title="puf" --timeout=7 &
sleep 3
`$sc -a puf -p test7.png`
