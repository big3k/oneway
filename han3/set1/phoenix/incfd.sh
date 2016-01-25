#!/bin/bash
echo "*   hard    nofile   16384" >> /etc/security/limits.conf
echo "ulimit -n 16384" >> /etc/bashrc
sed 's/^.*UsePrivilegeSeparation.*/UsePrivilegeSeparation no/' /etc/ssh/sshd_config > /tmp/abc
mv -f /tmp/abc /etc/ssh/sshd_config
/etc/init.d/sshd restart

