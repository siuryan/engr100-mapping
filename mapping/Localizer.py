import math

class Localizer:

    def __init__(self, logs, wall_threshold, accel_min_threshold, accel_max_threshold, accel_max_y_threshold, alpha):
        self.WALL_THRESHOLD = wall_threshold
        self.ACCEL_MIN_THRESHOLD = accel_min_threshold
        self.ACCEL_MAX_THRESHOLD = accel_max_threshold
        self.ACCEL_MAX_Y_THRESHOLD = accel_max_y_threshold
        self.ALPHA = alpha

        self.logs = logs
        self.walls = []  # List of tuples

        self.position_x = []
        self.position_y = []

        self.velocity_x = []
        self.velocity_y = []

        self._populate_velocity(logs)
        self._populate_position(logs)
        self._populate_walls(logs)


    def _populate_velocity(self, logs):
        self.velocity_x.append(0)
        self.velocity_y.append(0)

        for i in range(len(logs)-1):
            a_x = logs[i]['a_x'] # TODO: use angles
            a_y = logs[i]['a_y'] # TODO: use angles

            if abs(a_x) < self.ACCEL_MIN_THRESHOLD:
                a_x = 0
            if abs(a_x) > self.ACCEL_MAX_THRESHOLD:
                a_x = self.ACCEL_MAX_THRESHOLD * (abs(a_x)/a_x)

            if abs(a_y) < self.ACCEL_MIN_THRESHOLD:
                a_y = 0
            if abs(a_y) > self.ACCEL_MAX_Y_THRESHOLD:
                a_y = self.ACCEL_MAX_Y_THRESHOLD * (abs(a_y)/a_y)

            t1 = logs[i]['time']
            t2 = logs[i+1]['time']
            pitch = logs[i]['pitch']/100
            roll = logs[i]['roll']/100
            v_x = (a_x*(t2 - t1)*math.cos(math.radians(pitch)) - self.velocity_x[i]) * self.ALPHA + self.velocity_x[i]
            v_y = (a_y*(t2 - t1)*math.cos(math.radians(roll)) - self.velocity_y[i]) * self.ALPHA + self.velocity_y[i]
            # print (a_x, v_x)
            self.velocity_x.append(v_x)
            self.velocity_y.append(v_y)

    def _populate_position(self, logs):
        self.position_x.append(0)
        self.position_y.append(0)

        for i in range(len(logs)-1):
            v_x = self.velocity_x[i]
            v_y = self.velocity_y[i]

            # d_l1 = logs[i+1]['l1'] - logs[i]['l1']
            # d_l2 = logs[i+1]['l2'] - logs[i]['l2']
            # d_l3 = logs[i+1]['l3'] - logs[i]['l3']
            # d_l4 = logs[i+1]['l4'] - logs[i]['l4']

            # print (d_l1, d_l2, d_l3, d_l4)

            # if (abs(d_l1) < 0.01 or abs(d_l3) < 0.01) and abs(v_y) > 0.2:
            #     v_y = 0
            # if (abs(d_l2) < 0.01 or abs(d_l4) < 0.01) and abs(v_x) > 0.12:
            #     v_x = 0

            t1 = logs[i]['time']
            t2 = logs[i+1]['time']
            d_x = v_x*(t2 - t1) + self.position_x[i]
            d_y = v_y*(t2 - t1) + self.position_y[i]

            self.position_x.append(d_x)
            self.position_y.append(d_y)

    def _populate_walls(self, logs):
        # for i in range(len(logs)):
        #     pitch = logs[i]['pitch']/100
        #     roll = logs[i]['roll']/100
        #     l1 = logs[i]['l1']*math.cos(math.radians(pitch))
        #     l2 = logs[i]['l2']*math.cos(math.radians(roll))
        #     l3 = logs[i]['l3']*math.cos(math.radians(pitch))
        #     l4 = logs[i]['l4']*math.cos(math.radians(roll))

        #     # up
        #     if l1 < self.WALL_THRESHOLD:
        #         self.walls.append((self.position_x[i], self.position_y[i] + l1, i))
        #     # right
        #     if l2 < self.WALL_THRESHOLD:
        #         self.walls.append((self.position_x[i] + l2, self.position_y[i], i))
        #     # down
        #     if l3 < self.WALL_THRESHOLD:
        #         self.walls.append((self.position_x[i], self.position_y[i] - l3, i))
        #     # left
        #     if l4 < self.WALL_THRESHOLD:
        #         self.walls.append((self.position_x[i] - l4, self.position_y[i], i))

        pos_x = 0
        pos_y = 0

        self.position_x = []
        self.position_y = []

        for i in range(len(logs) - 1):
            pitch = logs[i]['pitch']/100
            roll = logs[i]['roll']/100
            l1 = logs[i]['l1']*math.cos(math.radians(pitch))
            l2 = logs[i]['l2']*math.cos(math.radians(roll))
            l3 = logs[i]['l3']*math.cos(math.radians(pitch))
            l4 = logs[i]['l4']*math.cos(math.radians(roll))

            d_l1 = logs[i+1]['l1']*math.cos(math.radians(pitch)) - l1
            d_l2 = logs[i+1]['l2']*math.cos(math.radians(roll)) - l2
            d_l3 = logs[i+1]['l3']*math.cos(math.radians(roll)) - l3
            d_l4 = logs[i+1]['l4']*math.cos(math.radians(roll)) - l4

            if l1 < self.WALL_THRESHOLD:
                pos_y += d_l1*3
            elif l3 < self.WALL_THRESHOLD:
                pos_y -= d_l3*3

            if l2 < self.WALL_THRESHOLD:
                pos_x -= d_l2*3
            elif l3 < self.WALL_THRESHOLD:
                pos_x += d_l4*3

            print (pos_x, pos_y)

            self.position_x.append(pos_x)
            self.position_y.append(pos_y)

            if l1 < self.WALL_THRESHOLD:
                self.walls.append((pos_x, pos_y + l1, i))
            # right
            if l2 < self.WALL_THRESHOLD:
                self.walls.append((pos_x + l2, pos_y, i))
            # down
            if l3 < self.WALL_THRESHOLD:
                self.walls.append((pos_x, pos_y - l3, i))
            # left
            if l4 < self.WALL_THRESHOLD:
                self.walls.append((pos_x - l4, pos_y, i))

    def get_walls(self):
        return self.walls

    def get_positions_x(self):
        return self.position_x

    def get_positions_y(self):
        return self.position_y
