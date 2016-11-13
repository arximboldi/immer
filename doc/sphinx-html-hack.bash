#!/bin/bash

location=`dirname $0`

# Fixes issues described here:
# https://github.com/michaeljones/breathe/issues/284

src='<span id="\([^"]*\)"></span>\(.*\)<em class="property">class </em>'
dst='<span id=""></span>\2<em class="property">class</em><tt class="descname">\1</tt>'
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='<span id="\([^"]*\)"></span>\(.*\)<em class="property">struct </em>'
dst='<span id=""></span>\2<em class="property">struct</em><tt class="descname">\1</tt>'
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='<em class="property">using</em><em class="property">using </em>'
dst='<em class="property">using </em>'
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='<em class="property">using </em><em class="property">using </em>'
dst='<em class="property">using </em>'
sed -i "s@$src@$dst@g" $location/_build/html/*.html

# src='<code class="descclassname">\([^&]*\)&lt;\([^&]*\)&gt;::</code>'
# dst='<code class="descclassname">\1::</code>'
src='<code class="descclassname">\([^<]*\)</code>'
dst=''
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='breathe-sectiondef container'
dst=''
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='typedef '
dst=''
sed -i "s@$src@$dst@g" $location/_build/html/*.html

src='<em class="property">using </em>template&lt;&gt;<br />'
dst=''
sed -i "s@$src@$dst@g" $location/_build/html/*.html
