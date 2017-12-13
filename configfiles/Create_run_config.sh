#!/bin/bash

cp -R template $1
more $1/ToolChainConfig | sed s:"configfiles/":"configfiles/"$1"/": > $1/tmp
mv $1/tmp $1/ToolChainConfig
ln -s  configfiles/$1/ToolChainConfig ../$1
