#!/bin/bash
#
# The script will run through multiple Kano Screenshot modes
# to make sure the results are as expected.
#
# The script must be run on the RaspberryPI, with kano-screenshot on your PATH.
#

# Fail immediately if kano-screenshot returns an error
set -e

# Handy placeholder to call the kano-screenshot tool
ks=$(which kano-screenshot)
vcgencmd=$(which vcgencmd)

# Test function which asserts that the execution of $1 returns the subliteral $2
# A short message explaining the test titled $3.
function kassert()
{
    $1 | grep $2 > /dev/null 2>&1
    if [ "$?" == "0" ]; then
        echo "Test passed: $3"
        return 0
    else
        echo "Test failed: $3 ('$1' does not contain: '$2')"
        return 1
    fi    
}

# width and height mode tests
`$ks -w 320 -p test10.png`
kassert "file test10.png" "320" "width mode"
`$ks -h 200 -p test11.png`
kassert "file test11.png" "200" "height mode"
`$ks -h 320x200 -p test12.png`
kassert "file test12.png" "320x200" "width/height mode"

# fullscreen mode test
# TODO: compare with "tvservice" to tell us the real display resolution
#`$ks -p test5.png`

# cropping
`$ks -c 10,10,175,300 -p test20.png`
kassert "file test20.png" "165x290" "even coordinates cropping mode"
`$ks -c 101,103,302,408 -p test21.png`
kassert "file test21.png" "202x305" "odd coordinates cropping mode"

# application mode
zenity --info "kano screenshot test application" --title="puf" --timeout=7 &
sleep 3
`$ks -a puf -p test30.png`
kassert "file test30.png" "202x305" "odd coordinates cropping mode"
