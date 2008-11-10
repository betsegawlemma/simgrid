
from twisted.application import service
from buildbot.slave.bot import BuildSlave

basedir = r'/var/lib/buildbot/simgrid/linux_amd64_ucontext'
host = 'bob.loria.fr'
port = 9989
slavename = 'bob_ucontext'
from slave_account import passwd # Separate file to put this one in the SVN
keepalive = 600
usepty = 1
umask = None

application = service.Application('buildslave')
s = BuildSlave(host, port, slavename, passwd, basedir, keepalive, usepty,
               umask=umask)
s.setServiceParent(application)

