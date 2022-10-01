---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: working-on-the-git-toolchain-repos
title: Working on the git toolchain repos
---

Everybody who might ever need to touch the repositories that were formerly

at git.chromium.org should start by doing the account setup that Anush

posted about: http://www.chromium.org/chromium-os/developer-guide/gerrit-guide

You can use your existing SSH key you were using with git.chromium.org

(that's what I did) or generate a fresh one if you prefer. Then you

should put this bit into your ~/.ssh/config:

Host gerrit.chromium.org

Port 29418

User &lt;your-gerrit-username&gt;

IdentityFile %d/.ssh/chromium

For sanity's sake, your gerrit username will be the same as your

chromium.org username which will be the same as your google.com username.

I did it that way even though I don't like my google.com username much

(and even though I am insane).

This config assumes that you are using your SSH key from before and/or that

you generated it with "ssh-keygen -f ~/.ssh/chromium". Adjust file names

to taste if you are nonconformist. Make sure that the bit you paste into

the account setup web form for your SSH key is instead the contents of

~/.ssh/chromium.pub (your public key, not your private key).

Once you have done the account setup, then you can ask someone to add you

to the nacl-toolchain-committers group. For the moment, you can ask me.

When other more adminy people get their gerrit accounts set up so I can

add them to the nacl-admin group, you should ask them instead of me.

The content for our repos has been migrated over from the

gitrw.chromium.org server, and nobody can push there anymore. Note that

the read-only http://git.chromium.org mirrors are still stale (lacking

two commits I pushed yesterday), and will probably never be updated again.

But that's OK!

The new repos are live. The read-only URLs are:

http://gerrit.chromium.org/gerrit/p/native_client/nacl-glibc.git

http://gerrit.chromium.org/gerrit/p/native_client/nacl-binutils.git

http://gerrit.chromium.org/gerrit/p/native_client/nacl-gcc.git

http://gerrit.chromium.org/gerrit/p/native_client/nacl-newlib.git

I will look into changing the toolchain builder crapola to pull from those.

The URLs for writing are:

ssh://gerrit.chromium.org/native_client/nacl-glibc.git

ssh://gerrit.chromium.org/native_client/nacl-binutils.git

ssh://gerrit.chromium.org/native_client/nacl-gcc.git

ssh://gerrit.chromium.org/native_client/nacl-newlib.git

To keep life simple, you can just do a fresh 'git clone' from one of the

ssh URLs. (You only need a gerrit account and not committer privs to be

able to clone that way.) It's also possible to set things up to pull from

http:// urls but push to ssh:// urls, but that is stranger and I don't

really know why you'd bother with it.

If you have an existing git checkout, you can fix the URLs just by changing

them in the .git/config file in each checkout. There is a way to do this

with the 'git config' command, but really I'd just edit the file. It ain't

rocket science.
