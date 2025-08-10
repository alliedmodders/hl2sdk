import sys
from ambuild2 import run

parser = run.BuildParser(sourcePath = sys.path[0], api='2.2')
parser.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug',
                        help='Enable debugging symbols')
parser.options.add_argument('--enable-optimize', action='store_const', const='1', dest='opt',
                        help='Enable optimization')
parser.options.add_argument('--targets', type=str, dest='targets', default=None,
		                help="Override the target architecture (use commas to separate multiple targets).")
parser.Configure()