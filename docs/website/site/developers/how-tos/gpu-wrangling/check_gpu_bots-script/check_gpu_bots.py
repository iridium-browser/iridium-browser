#!/usr/bin/env python

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import datetime
import getpass
import json
import os
import smtplib
import sys
import time
import urllib
import urllib2

BASEURL = 'http://build.chromium.org/p/'
BASEBUILDERURL = BASEURL + '%s/builders/%s'
BASEJSONBUILDERSURL = BASEURL + '%s/json/builders'
BASEJSONBUILDSURL = BASEURL + '%s/json/builders/%s/builds'
SPECIFICBUILDURL = BASEURL + '%s/builders/%s/builds/%s'

DEFAULTEMAILPASSWORDFILE = '.email_password'
DEFAULTPREVIOUSRESULTSFILE = '.poll_gpu_scripts_previous_results'

GMAILSMTPSERVER = 'smtp.gmail.com:587'

def getJsonFromUrl(url):
  c = urllib2.urlopen(url)
  js = c.read()
  c.close()
  return json.loads(js)

def getBuildersJsonForWaterfall(waterfall):
  querystring = '?filter'
  return getJsonFromUrl((BASEJSONBUILDERSURL + '%s') % (waterfall, querystring))

def getLastNBuildsForBuilder(n, waterfall, builder):
  if n == 0 or n < -1:
    return {}

  querystring = ''
  if n > 0:
    querystring = '?'
    for i in range(n):
      querystring += 'select=-%d&' % (i + 1)
    querystring += 'filter'

  return getJsonFromUrl((BASEJSONBUILDSURL + '%s') %
          (waterfall, urllib.quote(builder), querystring))

def getFilteredBuildersJsonForWaterfall(waterfall, filter):
  querystring = '?'
  for botindex in range(len(filter)):
    bot = filter[botindex]
    querystring += 'select=%s&' % urllib.quote(bot)
  querystring += 'filter'

  return getJsonFromUrl((BASEJSONBUILDERSURL + '%s') % (waterfall, querystring))

def getAllGpuBots():
  regularwaterfalls = ['chromium.gpu',
          'tryserver.chromium.gpu',
          'chromium.gpu.fyi']

  WEBKITGPUBOTS = ['GPU Win Builder',
          'GPU Win Builder (dbg)',
          'GPU Win7 (NVIDIA)',
          'GPU Win7 (dbg) (NVIDIA)',
          'GPU Mac Builder',
          'GPU Mac Builder (dbg)',
          'GPU Mac10.7',
          'GPU Mac10.7 (dbg)',
          'GPU Linux Builder',
          'GPU Linux Builder (dbg)',
          'GPU Linux (NVIDIA)',
          'GPU Linux (dbg) (NVIDIA)']
  filteredwaterfalls = [('chromium.webkit', WEBKITGPUBOTS)]

  allbots = {k: getBuildersJsonForWaterfall(k) for k in regularwaterfalls}
  filteredbots = {k[0]: getFilteredBuildersJsonForWaterfall(k[0], k[1])
          for k in filteredwaterfalls}
  allbots.update(filteredbots)

  return allbots

def timestr(t):
  return time.strftime("%a, %d %b %Y %H:%M:%S", t)

def roughTimeDiffInHours(t1, t2):
  dt = []
  for t in [t1, t2]:
    dt.append(datetime.datetime(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour,
        t.tm_min, t.tm_sec))
  dtd = dt[0] - dt[1]

  hours = float(dtd.total_seconds()) / 3600.0

  return abs(hours)

def getBotStr(bot, t):
  s = '  %s::%s\n' % (bot['waterfall'], bot['name'])

  if 'failurestring' in bot:
    s += '  failure: %s\n' % bot['failurestring']
  if 'endtime' in bot:
    s += ('  last build end time: %s (roughly %f hours ago)\n' %
    (timestr(bot['endtime']), roughTimeDiffInHours(t, bot['endtime'])))
  if 'boturl' in bot:
    s += '  bot url: %s\n' % bot['boturl']
  if 'buildurl' in bot:
    s += '  build url: %s\n' % bot['buildurl']
  s += '\n'

  return s

def getBotsStr(bots):
  t = time.localtime()
  s = ''
  for bot in bots:
    s += getBotStr(bot, t)
  s += '\n'
  return s

def getOfflineBots(bots):
  offlinebots = []
  for waterfallname in bots:
    waterfall = bots[waterfallname]
    for botname in waterfall:
      bot = waterfall[botname]
      if bot['state'] != 'offline':
        continue
      botspec = {'waterfall': waterfallname, 'name': botname, 'impl': bot}
      mrbuild = getMostRecentlyCompletedBuildForBot(botspec)
      if mrbuild:
        botspec['endtime'] = time.localtime(mrbuild['times'][1])
        botspec['boturl'] = BASEBUILDERURL % (waterfallname,
                urllib.quote(botname))
      else:
        print 'no most recent build available: %s::%s' % (waterfallname,
                botname)
      offlinebots.append(botspec)
  return offlinebots

def getOfflineBotsStr(offlinebots):
  return 'Offline bots:\n%s' % getBotsStr(offlinebots)

def getMostRecentlyCompletedBuildForBot(bot):
  if 'impl' in bot and 'mrbuild' in bot['impl']:
    return bot['impl']['mrbuild']

  numbuilds = 10 # TODO?
  builds = getLastNBuildsForBuilder(numbuilds, bot['waterfall'], bot['name'])
  for i in range(numbuilds):
    currbuildname = '-%d' % (i + 1)
    currbuild = builds[currbuildname]
    if 'results' in currbuild and currbuild['results'] is not None:
      if 'impl' in bot:
        bot['impl']['mrbuild'] = currbuild
      return currbuild

def getFailedBots(bots):
  failedbots = []
  for waterfallname in bots:
    waterfall = bots[waterfallname]
    for botname in waterfall:
      bot = waterfall[botname]
      botspec = {'waterfall': waterfallname, 'name': botname, 'impl': bot}
      mrbuild = getMostRecentlyCompletedBuildForBot(botspec)
      if mrbuild and 'text' in mrbuild and 'failed' in mrbuild['text']:
        botspec['failurestring'] = ' '.join(mrbuild['text'])
        botspec['boturl'] = BASEBUILDERURL % (waterfallname,
                urllib.quote(botname))
        botspec['buildurl'] = SPECIFICBUILDURL % (waterfallname,
                urllib.quote(botname), mrbuild['number'])
        failedbots.append(botspec)
      elif not mrbuild:
        print 'no most recent build available: %s::%s' % (waterfallname,
                botname)
  return failedbots

def getFailedBotsStr(failedbots):
  return 'Failed bots:\n%s' % getBotsStr(failedbots)

def getShallowBots(bots):
  shallowbots = []
  for bot in bots:
    shallowbot = {}
    for k in bot:
      if k != 'impl':
        shallowbot[k] = bot[k]
    shallowbots.append(shallowbot)
  return shallowbots

def jsonableToTime(j):
  return time.struct_time((j['year'], j['mon'], j['day'], j['hour'], j['min'],
      j['sec'], 0, 0, 0))

# mutates bots
def offlineBotsJsonableToTime(bots):
  for bot in bots['offline']:
    bot['endtime'] = jsonableToTime(bot['endtime'])
  return bots

def getPreviousResults(previous_results_file):
  file = (previous_results_file if previous_results_file is not None
          else DEFAULTPREVIOUSRESULTSFILE)
  previous_results = {}
  if os.path.isfile(file):
    with open(file, 'r') as f:
      previous_results = offlineBotsJsonableToTime(json.loads(f.read()))
  return previous_results

def timeToJsonable(t):
  return {'year': t.tm_year, 'mon': t.tm_mon, 'day': t.tm_mday,
          'hour': t.tm_hour, 'min': t.tm_min, 'sec': t.tm_sec}

# this mutates the offline bots, do it last
def offlineBotsTimeToJsonable(bots):
  t = time.localtime()
  for bot in bots:
    bot['hourssincelastrun'] = roughTimeDiffInHours(bot['endtime'], t)
    bot['endtime'] = timeToJsonable(bot['endtime'])
  return bots

def getSummary(offlinebots, failedbots):
  shallowofflinebots = offlineBotsTimeToJsonable(getShallowBots(offlinebots))
  shallowfailedbots = getShallowBots(failedbots)
  return {'offline': shallowofflinebots, 'failed': shallowfailedbots}

def writeResults(summary, previous_results_file):
  file = (previous_results_file if previous_results_file is not None
          else DEFAULTPREVIOUSRESULTSFILE)
  with open(file, 'w') as f:
    f.write(json.dumps(summary))

def findBot(name, lst):
  for bot in lst:
    if bot['name'] == name:
      return bot
  return None

def getNoteworthyEvents(offlinebots, failedbots, previous_results):
  t = time.localtime()

  CRITICALNUMHOURS = 1.0

  previousoffline = (previous_results['offline'] if 'offline'
          in previous_results else [])
  previousfailures = (previous_results['failed'] if 'failed'
          in previous_results else [])

  noteworthyoffline = []
  for bot in offlinebots:
    if roughTimeDiffInHours(bot['endtime'], t) > CRITICALNUMHOURS:
      previousbot = findBot(bot['name'], previousoffline)
      if (previousbot is None or
              previousbot['hourssincelastrun'] < CRITICALNUMHOURS):
        noteworthyoffline.append(bot)

  noteworthynewfailures = []
  for bot in failedbots:
    previousbot = findBot(bot['name'], previousfailures)
    if previousbot is None:
      noteworthynewfailures.append(bot)

  noteworthynewofflinerecoveries = []
  for bot in previousoffline:
    if bot['hourssincelastrun'] < CRITICALNUMHOURS:
      continue
    currentbot = findBot(bot['name'], offlinebots)
    if currentbot is None:
      noteworthynewofflinerecoveries.append(bot)

  noteworthynewfailurerecoveries = []
  for bot in previousfailures:
    currentbot = findBot(bot['name'], failedbots)
    if currentbot is None:
      noteworthynewfailurerecoveries.append(bot)

  return {'offline': noteworthyoffline, 'failed': noteworthynewfailures,
          'recoveredfailures': noteworthynewfailurerecoveries,
          'recoveredoffline': noteworthynewofflinerecoveries}

def getNoteworthyStr(noteworthy):
  t = time.localtime()

  s = ''

  if noteworthy['offline']:
    s += 'IMPORTANT bots newly offline for over an hour:\n'
    for bot in noteworthy['offline']:
      s += getBotStr(bot, t)
    s += '\n'

  if noteworthy['failed']:
    s += 'IMPORTANT new failing bots:\n'
    for bot in noteworthy['failed']:
      s += getBotStr(bot, t)
    s += '\n'

  if noteworthy['recoveredoffline']:
    s += 'IMPORTANT newly recovered previously offline bots:\n'
    for bot in noteworthy['recoveredoffline']:
      s += getBotStr(bot, t)
    s += '\n'

  if noteworthy['recoveredfailures']:
    s += 'IMPORTANT newly recovered failing bots:\n'
    for bot in noteworthy['recoveredfailures']:
      s += getBotStr(bot, t)
    s += '\n'

  return s

def hasEmailable(noteworthy, send_email_for_recovered_offline_bots,
        send_email_for_recovered_failing_bots):
  if noteworthy['offline'] or noteworthy['failed']:
    return True

  if send_email_for_recovered_offline_bots and noteworthy['recoveredoffline']:
    return True

  if (send_email_for_recovered_failing_bots and
        noteworthy['recoveredfailures']):
    return True

  return False

def get_email_body(tstr, offlinestr, failedstr, noteworthystr):
  return '%s%s%s%s' % (tstr, offlinestr, failedstr, noteworthystr)

def send_email(email_from, email_to, email_password, body):
  subject = 'Chrome GPU Bots Notification'
  message = 'From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s' % (email_from,
          ','.join(email_to), subject, body)

  try:
    server = smtplib.SMTP(GMAILSMTPSERVER)
    server.starttls()
    server.login(email_from, email_password)
    server.sendmail(email_from, email_to, message)
    server.quit()
  except Exception as e:
    print 'Error sending email: %s' % str(e)

def testEmailLogin(email_from, email_password):
  server = smtplib.SMTP(GMAILSMTPSERVER)
  server.starttls()
  server.login(email_from, email_password)
  server.quit()

def getEmailPassword(email_password_file):
  password = ''

  password_file = (email_password_file if email_password_file is not None
          else DEFAULTEMAILPASSWORDFILE)

  if os.path.isfile(password_file):
    with open(password_file) as f:
      password = f.read().strip()
  else:
    password = getpass.getpass(
            'Please enter email password for source account: ')

  return password

def checkBots(email_from, email_to, send_email_for_recovered_offline_bots,
        send_email_for_recovered_failing_bots, send_email_on_error,
        email_password, previous_results_file):
  tstr = 'Current time: %s\n\n' % (timestr(time.localtime()))
  print tstr

  try:
    # TODO(hartmanng) parallelize the http requests
    bots = getAllGpuBots()

    offlinebots = getOfflineBots(bots)
    offlinestr = getOfflineBotsStr(offlinebots)
    print offlinestr

    failedbots = getFailedBots(bots)
    failedstr = getFailedBotsStr(failedbots)
    print failedstr

    previous_results = getPreviousResults(previous_results_file)
    noteworthy = getNoteworthyEvents(offlinebots, failedbots, previous_results)
    noteworthystr = getNoteworthyStr(noteworthy)
    print noteworthystr

    # this mutates the offline bots, do it last
    summary = getSummary(offlinebots, failedbots)
    writeResults(summary, previous_results_file)

    if (email_from is not None and email_to is not None and
            hasEmailable(noteworthy, send_email_for_recovered_offline_bots,
                send_email_for_recovered_failing_bots)):
      send_email(email_from, email_to, email_password,
              get_email_body(tstr, offlinestr, failedstr, noteworthystr))
  except Exception as e:
    errorstr = 'Error: %s' % str(e)
    print errorstr
    if send_email_on_error:
      send_email(email_from, email_to, email_password, errorstr)

def parseArgs(sysargs):
  parser = argparse.ArgumentParser(prog=sysargs[0],
          description='Query the Chromium GPU Bots Waterfall, output ' +
          'potential problems, and optionally repeat automatically and/or ' +
          'email notifications of results.')
  parser.add_argument('--repeat-delay', type=int, dest='repeat_delay',
          required=False,
          help='How often to automatically re-run the script, in minutes.')
  parser.add_argument('--email-from', type=str, dest='email_from',
          required=False,
          help='Email address to send from. Requires also specifying ' +
          '\'--email-to\'.')
  parser.add_argument('--email-to', type=str, dest='email_to', required=False,
          nargs='+',
          help='Email address(es) to send to. Requires also specifying ' +
          '\'--email-from\'')
  parser.add_argument('--send-email-for-recovered-offline-bots',
          dest='send_email_for_recovered_offline_bots', action='store_true',
          default=False,
          help='Send an email out when a bot which has been offline for more ' +
          'than 1 hour goes back online.')
  parser.add_argument('--send-email-for-recovered-failing-bots',
          dest='send_email_for_recovered_failing_bots',
          action='store_true', default=False,
          help='Send an email when a failing bot recovers.')
  parser.add_argument('--send-email-on-error',
          dest='send_email_on_error',
          action='store_true', default=False,
          help='Send an email when the script has an error. For example, if ' +
          'the server is unreachable.')
  parser.add_argument('--email-password-file',
          dest='email_password_file',
          required=False,
          help=(('File containing the plaintext password of the source email ' +
          'account. By default, \'%s\' will be tried. If it does not exist, ' +
          'you will be prompted. If you opt to store your password on disk ' +
          'in plaintext, use of a dummy account is strongly recommended.')
          % DEFAULTEMAILPASSWORDFILE))
  parser.add_argument('--previous-results-file',
          dest='previous_results_file',
          required=False,
          help=(('File to store the results of the previous invocation of ' +
              'this script. By default, \'%s\' will be used.')
              % DEFAULTPREVIOUSRESULTSFILE))

  args = parser.parse_args(sysargs[1:])

  if args.email_from is not None and args.email_to is None:
    parser.error('--email-from requires --email-to.')
  elif args.email_to is not None and args.email_from is None:
    parser.error('--email-to requires --email-from.')
  elif args.email_from is None and args.send_email_for_recovered_offline_bots:
    parser.error('--send-email-for-recovered-offline-bots requires ' +
            '--email-to and --email-from.')
  elif (args.email_from is None and args.send_email_for_recovered_failing_bots):
    parser.error('--send-email-for-recovered-failing-bots ' +
            'requires --email-to and --email-from.')
  elif (args.email_from is None and args.send_email_on_error):
    parser.error('--send-email-on-error ' +
            'requires --email-to and --email-from.')
  elif (args.email_password_file and
          not os.path.isfile(args.email_password_file)):
    parser.error('File does not exist: %s', args.email_password_file)

  return args

def main(sysargs):
  args = parseArgs(sysargs)

  email_password = None
  if args.email_from is not None and args.email_to is not None:
    email_password = getEmailPassword(args.email_password_file)
    try:
      testEmailLogin(args.email_from, email_password)
    except Exception as e:
      print 'Error logging into email account: %s' % str(e)
      sys.exit(1)

  while True:
    checkBots(args.email_from, args.email_to,
            args.send_email_for_recovered_offline_bots,
            args.send_email_for_recovered_failing_bots,
            args.send_email_on_error,
            email_password,
            args.previous_results_file)
    if args.repeat_delay is None:
      break
    print 'Will run again in %d minutes...\n' % args.repeat_delay
    time.sleep(args.repeat_delay * 60)

if __name__ == '__main__':
  main(sys.argv)
