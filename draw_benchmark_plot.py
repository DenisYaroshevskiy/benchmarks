import argparse
import json
import plotly
import re
import itertools

from plotly.graph_objs import Scatter, Layout

class Measurement:
  def __init__(self, input_size, time):
    self.input_size = input_size
    self.time = time

class Title:
  def __init__(self, set_size, distribution_size):
    self.set_size = set_size
    self.distribution_size = distribution_size

def parseMeasurement(json_dict):
  parsed_name = re.match(r'(.*?)(_solution)?/(.*)',json_dict['name'])
  name = parsed_name.group(1)
  name = name.replace('_', ' ')
  if name == 'proposed':
    name += ' solution'
  print name

  input_size = int(parsed_name.group(3))
  time = json_dict['real_time']
  return (name, Measurement(input_size, time))

def parseJson(loaded_benchmarks_json, baseline_name):
  measurements = {}
  title = None
  for json_input in loaded_benchmarks_json['benchmarks']:
    name, m = parseMeasurement(json_input)
    if 'extra_data' in json_input:
      set_size = json_input['extra_data']["set_size"]
      distribution_size = json_input['extra_data']["distribution_size"]
      title = Title(set_size, distribution_size)
    measurements.setdefault(name, []).append(m)

  if baseline_name not in measurements:
    raise "No baseline benchmark found"

  for name, ms in measurements.iteritems():
    if name == baseline_name:
      continue
    for m, baseline in zip(ms, measurements[baseline_name]):
      assert(m.input_size == baseline.input_size)
      m.time - baseline.time

  del measurements[baseline_name]

  return (title, measurements)

def drawPlot(title, measurements):
  traces = []
  for method, results in measurements.iteritems():
    input_sizes = map(lambda m: m.input_size, results)
    times = map(lambda m: m.time, results)
    trace = Scatter(x = input_sizes, y = times, mode = 'lines', name = method)
    traces.append(trace)

  data = plotly.graph_objs.Data(traces)
  layout = {}
  layout['title'] = \
    '<b>Benchmarking flat_set<int>::insert(first, last)</b><br>' + \
    'set size : ' + str(title.set_size) + \
    ' value distribution(1..' + str(title.distribution_size) + ')'

  layout['xaxis'] = dict(title='distance(first, last)')
  layout['yaxis'] = dict(title='ns')

  return plotly.plotly.plot(
    dict(data=data, layout=layout),
    filename = 'flat_set_insert_1000_unlikely_duplicates',
    fileopt = 'overwrite',
    auto_open = False,
  )

if __name__ == "__main__":
  options_parser = argparse.ArgumentParser( \
        description='Comparing performance of different implementations.')

  options_parser.add_argument('--benchmarks_result_json',
                               dest='benchmarks_result_json',
                               required=True)
  options_parser.add_argument('--baseline_name',
                               dest='baseline_name',
                               default='baseline')
  options = options_parser.parse_args()
  baseline_name = options.baseline_name
  loaded_benchmarks = json.load(open(options.benchmarks_result_json))

  title, measurements = parseJson(loaded_benchmarks, baseline_name)
  print drawPlot(title, measurements)


