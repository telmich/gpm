#!/bin/sh
# 
# 2008 Nico Schottelius (nico-ccollect at schottelius.org)
# 
# This file is part of ccollect.
#
# ccollect is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# ccollect is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with ccollect. If not, see <http://www.gnu.org/licenses/>.
#
# Sends feedback
#
# Adapted for gpm
#

software="gpm"
author="Nico Schottelius"
subject="==> success: $software"
to="nico-gpm-success"
host="schottelius.org"
fullto="${to}@${host}"

info="$(awk -F= '/^release=/ { print $2 }' configure.in; uname -s -v -r -m)"


echo "Reporting success for $software to ${author}"
echo "-----------------"
echo -n "Your name (leave free for anonymous): "
read name
echo -n "Your email (leave free for anonymous): "
read email
echo -n "Your normal gpm parameters (-m -t -o ...): "
read params
echo -n "Comment (leave free for no comment): "
read comment

echo ""
echo "The following information will be send to ${author}:"
echo ""

cat << eof
Name: $name (will be used to contact you and kept secret)
E-Mail: $email (will be used to contact you and kept secret)
Comment: $comment
Info: $info
Parameters: $params

eof

echo -n "Is it ok to send out that mail (press enter to send or ctrl-c to abort)? "
read yes

cat << eof | mail -s "$subject" "$fullto"
Name: $name
E-Mail: $email
Comment: $comment
Info: $info
Parameters: $params
eof

echo "Send. Thank you for your feedback."
