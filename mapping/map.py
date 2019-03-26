from Plotter import Plotter
from LogReader import LogReader

def main():
    plotter = Plotter()
    log_reader = LogReader()

    log_reader.read_logs('sample-log.txt', True)
    print (log_reader.get_logs())

    # TODO: replace this with points from log
    for i in range(10):
        plotter.add_point(i, i)

    plotter.plot()

if __name__ == "__main__":
    main()
