---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/autotest-developer-faq
  - Autotest Developer FAQ
page_name: setup-autotest-server
title: Setup Autotest Server
---

This document will go over how to set up a local Autotest server for developing
purposes.

**Note that there is a script which will automate this process:
src/third_party/autotest/files/site_utils/setup_dev_autotest.sh**

You need to manually edit the generated shadow_config.ini and fill in your dev
server hostname.

You need to set up SSH for the apache user (www-data) in its home directory.
Follow the directions for ssh key setup, using the directory /var/www/.ssh and
make sure everything is owned by www-data with user only permissions:
<https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md#Set-up-SSH-connection-between-chroot-and-DUT>

If you ran the setup script please proceed to the Autotest Server Usage Guide:
<https://www.chromium.org/chromium-os/testing/autotest-developer-faq/autotest-server-usage>

You will want to re-run this script or the below steps for changes that are not
easily testable in the production server. Generally this will not be needed for
test development. This is if you are changing RPCs, The Scheduler, Any of the
frontend GUI or common library changes.

Reference: <https://github.com/autotest/autotest/wiki/AutotestServerInstall>

**Note that the following may be out of date.**

# Debian/Ubuntu

These instructions will only work with a Debian based installation. Most likely
you can use the package names to set up Autotest for Fedora/Redhat.

## Base deb packages to install

After installing all of the packages below you will end up with a server running
MySQL, Apache, and have the supporting libraries need to bootstrap the Autotest
installation.

\*\*Be sure to remember the password you set up for the root user in Mysql (If
any)

```none
 sudo apt-get install mysql-server mysql-common libapache2-mod-wsgi python-mysqldb gnuplot apache2-mpm-prefork unzip python-imaging libpng12-dev libfreetype6-dev sqlite3 python-pysqlite2 git-core pbzip2 openjdk-6-jre openjdk-6-jdk python-crypto  python-dev subversion build-essential python-setuptools
```

***\*Note you will be prompted to enter a 'root' user password for MySQL this is
reserved for administering the MySQL instance itself. You should not use this
for the next section but do not forget it either.***

## Setup MySQL DBs

Autotest use MySQL for all of its result storing and queuing jobs for the
scheduler to interface with.

Setup the Autotest MYSQL user:

<pre><code>
mysql -u root -p
create database chromeos_autotest_db;
grant all privileges on chromeos_autotest_db.* TO 'chromeosqa-admin'@'localhost' identified by '<b>some_password</b>'; 
FLUSH PRIVILEGES;
</code></pre>

***\*some_password can be whatever you want. Make note of it as you will need it
later on.***

## Setup Autotest Directory on the server

NOTE: Do not try to make the autotest server run directly from your home
directory and DO follow the "/usr/local/autotest" bind mount or checkout.
Changing all the paths in the source and making it work at the same time seems
downright impossible.

\*Preferred way, if you have a Chromium OS checkout you should already have the
Autotest soure.

```none
CHECKOUT=/your/chromiumos/checkout
sudo mkdir /usr/local/autotest
sudo mount --bind $CHECKOUT/src/third_party/autotest/files /usr/local/autotest
```

If you don't have a Chromium OS checkout and just want the Autotest bits:

```none
mkdir /usr/local/autotest
git clone https://chromium.googlesource.com/chromiumos/third_party/autotest.git /usr/local/autotest
```

Edit the *global_config.ini,* or better yet, create a shadow configuration
(place it in *shadow_config.ini)* to update the following areas:

**\[AUTOTEST_WEB\]**

*   host, set it to **localhost**
*   user, **chromeosqa-admin**, as set above
*   password, set it to the password you set, in this doc that would be
            ***some_password***
*   readonly_host, set it to **localhost**
*   readonly_user, set it to **chromeosqa-admin**
*   readonly_password, set this to the password you set, in this doc
            that would be **some_password**

**\[SERVER\]**

*   hostname, set it to localhost

**\[SCHEDULER\]**

*   drones, set it to localhost

#### Build external packages for use in Autotest

# Run this as the user you want to run the Autotest server as.

```none
/usr/local/autotest/utils/build_externals.py
/usr/local/autotest/utils/compile_gwt_clients.py -a
```

#### Setup Initial db

```none
/usr/local/autotest/database/migrate.py sync
/usr/local/autotest/frontend/manage.py syncdb
# You may have to run this twice. 
/usr/local/autotest/frontend/manage.py syncdb
```

## Setup Apache

If you ran the above package install command you should already have a running
apache server. All we need to do is point that configuration at the Autotest
configuration.

```none
sudo ln -s /usr/local/autotest/apache/apache-conf /etc/apache2/sites-available/autotest-server.conf
# disable currently active default
sudo a2dissite 000-default
# enable autotest server
sudo a2ensite autotest-server
# Enable rewrite module
sudo a2enmod rewrite
# Enable version for IfVersion code (may not be needed anymore)
sudo a2enmod version
# Disable mod-python
sudo a2dismod python
# Enable mod-wsgi
sudo a2enmod wsgi
# Enable mod_headers
sudo a2enmod headers
# Setup permissions so that Apache web user can read the proper files. 
chmod -R o+r /usr/local/autotest
find /usr/local/autotest/ -type d | xargs chmod o+x
chmod o+x /usr/local/autotest/tko/*.cgi
# restart server
sudo /etc/init.d/apache2 restart
```

If you try http://localhost, and Autotest afe page fails to load, you might want
to check the apache's port configuration file (/etc/apache2/ports.conf) to make
sure the file has port 80 listed:

```none
NameVirtualHost *:80
Listen 80
```

Otherwise, apache will not be listening on port 80.

If you get 503 errors and see the following message in
/var/log/apache2/error.log:

(2)No such file or directory: mod_wsgi: Couldn't bind unix domain socket
'/etc/apache2/run/wsgi.30365.0.1.sock'.

Just create the /etc/apache2/run directory.

## Viewing test logs from jobs ran via the scheduler

To view logs of a currently running job from the AFE click on the "Debug logs"
link. From there you have a few different choices, you will most likely want to
look at all the logs which are all in autoserv.DEBUG. Although this file does
not automatically update when viewed in your browser if you refresh your view
you will see the progress of the job.

All logs from jobs run via the scheduler are stored under:
/usr/local/autotest/results.

If you get 403 Forbidden when viewing the results, or if the AFE starts giving
Error 500 and strugging to import code, then you either need to re-run

```none
# Setup permissions so that Apache web user can read the proper files. 
chmod -R o+r /usr/local/autotest
find /usr/local/autotest/ -type d | xargs chmod o+x
chmod o+x /usr/local/autotest/tko/*.cgi
```

Or (recommended) edit /etc/apache2/envvars so that it reads

```none
export APACHE_RUN_USER=<user>
export APACHE_RUN_GROUP=<group>
```

where &lt;user&gt; is what is returned by running |id -un| and &lt;group&gt; is
the result of running |id -gn|

## Import Tests

Import tests

```none
/usr/local/autotest/utils/test_importer.py
```

**\*\*\*This is the end of what is done for you by
setup_dev_autotest.sh.\*\*\***

## Using the Autotest Server

Now that everything is setup, please proceed to the Autotest Server Usage Guide
<https://www.chromium.org/chromium-os/testing/autotest-developer-faq/autotest-server-usage>
