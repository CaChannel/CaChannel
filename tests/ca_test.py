from CaChannel import ca
import unittest

class CaTest(unittest.TestCase):
    def __init__(self, testName, chanName):
        unittest.TestCase.__init__(self, testName)
        self.chanName = chanName

    def assertNormal(self, status):
        self.assertEqual(status, ca.ECA_NORMAL, "%s: %s" % (self.chanName, ca.message(status)))

    def assertValueEqual(self, first, second):
        if isinstance(first, float) or isinstance(second, float):
            self.assertAlmostEqual(first, second, places=5)
        elif hasattr(first, 'all') or hasattr(second, 'all'):
            self.assertTrue((first==second).all())
        else:
            self.assertEqual(first, second)

class CaCreateTest(CaTest):

    def test_create(self):
        status, chid = ca.create_channel(self.chanName)
        self.assertNormal(status)
        status = ca.pend_io(3)
        self.assertNormal(status)
        self.assertTrue(ca.state(chid) == ca.cs_conn)
        status = ca.clear_channel(chid)
        self.assertNormal(status)

    def test_create_callback(self):
        epicsArgs = {}
        def connectionCB(args):
            for k,v in args.items():
                epicsArgs[k] = v

        status, chid = ca.create_channel(self.chanName, connectionCB)
        self.assertNormal(status)
        ca.pend_event(1)
        self.assertEqual(ca.state(chid), ca.cs_conn)
        self.assertEqual(epicsArgs['op'], ca.CA_OP_CONN_UP)
        status = ca.clear_channel(chid)
        self.assertNormal(status)

class CaGetTest(CaTest):

    def __init__(self, testName, chanName, dbrType, value, use_numpy=False):
        unittest.TestCase.__init__(self, testName)
        self.chanName = chanName
        self.dbrType = dbrType
        self.value = value
        self.use_numpy = use_numpy

    def setUp(self):
        status, chid = ca.create_channel(self.chanName)
        self.assertNormal(status)
        status = ca.pend_io(3)
        self.assertNormal(status)
        self.assertTrue(ca.state(chid) == ca.cs_conn)
        self.chid = chid

    def test_get(self):
        status, dbrValue = ca.get(self.chid, chtype=self.dbrType, use_numpy=self.use_numpy)
        self.assertNormal(status)
        status = ca.pend_io(1)
        self.assertNormal(status)
        value = dbrValue.get()
        self.checkValue(value)

    def test_get_callback(self):
        epicsArgs = {}
        done = [False]

        def getCB(args):
            for k,v in args.items():
                epicsArgs[k] = v
            done[0] = True

        status, value = ca.get(self.chid, callback=getCB, chtype=self.dbrType, use_numpy=self.use_numpy)
        self.assertNormal(status)
        self.assertTrue(value is None)
        while not done[0]:
            ca.pend_event(0.05)
        self.assertNormal(epicsArgs['status'])
        self.checkValue(epicsArgs['value'])

    def test_monitor(self):
        epicsArgs = {}
        done = [False]

        def monCB(args):
            for k,v in args.items():
                epicsArgs[k] = v
            done[0] = True

        status, evid = ca.create_subscription(self.chid, callback=monCB, chtype=self.dbrType, use_numpy=self.use_numpy)
        self.assertNormal(status)
        while not done[0]:
            ca.pend_event(0.05)
        self.assertNormal(epicsArgs['status'])
        self.checkValue(epicsArgs['value'])
        status = ca.clear_subscription(evid)
        self.assertNormal(status)
        ca.pend_event(0.05)

    def checkValue(self, value):
        if ca.dbr_type_is_plain(self.dbrType):
            self.assertValueEqual(value, self.value)
        else:
            self.assertTrue('status' in value)
            self.assertTrue('severity' in value)
            self.assertValueEqual(value['value'], self.value)
            if ca.dbr_type_is_TIME(self.dbrType):
                self.assertTrue('stamp' in value)
            if ca.dbr_type_is_STRING(self.dbrType):
                return
            if ca.dbr_type_is_GR(self.dbrType) or ca.dbr_type_is_CTRL(self.dbrType):
                if ca.dbr_type_is_ENUM(self.dbrType):
                    self.assertTrue('no_str' in value)
                    self.assertTrue('strs' in value)
                else:
                    self.assertTrue('status' in value)
                    self.assertTrue('severity' in value)
                    self.assertTrue('units' in value)
                    self.assertTrue('upper_disp_limit' in value)
                    self.assertTrue('lower_disp_limit' in value)
                    self.assertTrue('upper_warning_limit' in value)
                    self.assertTrue('lower_warning_limit' in value)
                    self.assertTrue('upper_alarm_limit' in value)
                    self.assertTrue('lower_alarm_limit' in value)
                    if ca.dbr_type_is_FLOAT(self.dbrType) or ca.dbr_type_is_DOUBLE(self.dbrType):
                        self.assertTrue('precision' in value)
                    if ca.dbr_type_is_CTRL(self.dbrType):
                        self.assertTrue('upper_ctrl_limit' in value)
                        self.assertTrue('lower_ctrl_limit' in value)

    def tearDown(self):
        ca.clear_channel(self.chid)
        ca.flush_io()


class CaPutTest(CaTest):

    def __init__(self, testName, chanName, dbrType, count, value, readback=None):
        unittest.TestCase.__init__(self, testName)
        self.chanName = chanName
        self.dbrType = dbrType
        self.count = count
        self.value = value
        if readback is None:
            self.readback = value
        else:
            self.readback = readback

    def setUp(self):
        status, chid = ca.create_channel(self.chanName)
        self.assertNormal(status)
        status = ca.pend_io(3)
        self.assertNormal(status)
        self.assertTrue(ca.state(chid) == ca.cs_conn)
        self.chid = chid

    def test_put(self):
        status = ca.put(self.chid, self.value, chtype=self.dbrType, count=self.count)
        self.assertNormal(status)
        status = ca.flush_io()
        self.assertNormal(status)

        status, dbrValue = ca.get(self.chid, chtype=self.dbrType, count=self.count)
        self.assertNormal(status)
        status = ca.pend_io(1)
        self.assertNormal(status)
        value = dbrValue.get()
        self.assertValueEqual(value, self.readback)

    def test_put_callback(self):
        epicsArgs = {}
        done = [False]

        def putCB(args):
            for k,v in args.items():
                epicsArgs[k] = v
            done[0] = True

        status = ca.put(self.chid, self.value, callback=putCB, chtype=self.dbrType, count=self.count)
        self.assertNormal(status)
        while not done[0]:
            ca.pend_event(0.05)
        self.assertNormal(epicsArgs['status'])

    def tearDown(self):
        ca.clear_channel(self.chid)
        ca.flush_io()

class CaGroupTest(CaTest):

    def __init__(self, testName, channels):
        unittest.TestCase.__init__(self, testName)
        self.chanName = ''
        self.channels = []
        for chanName, dbrType, value in channels:
            self.channels.append([None, chanName, dbrType, value, None])

    def setUp(self):
        for channel in self.channels:
            _, chanName, dbrType, value, _ = channel
            status, chid = ca.create_channel(chanName)
            self.assertNormal(status)
            channel[0] = chid

        status = ca.pend_io(3)
        self.assertNormal(status)

        status, self.gid = ca.sg_create()
        self.assertNormal(status)

    def test_group(self):
        for chid, chanName, dbrType, value, _ in self.channels:
            status = ca.sg_put(self.gid, chid, value, dbrType)
            self.assertNormal(status)
        while ca.sg_test(self.gid)  != ca.ECA_IODONE:
            ca.sg_block(self.gid, 0.05)

        for channel in self.channels:
            chid, chanName, dbrType, value, _ = channel
            status, dbrValue = ca.sg_get(self.gid, chid, dbrType)
            self.assertNormal(status)
            channel[-1] = dbrValue

        while ca.sg_test(self.gid)  != ca.ECA_IODONE:
            ca.sg_block(self.gid, 0.05)

        for chid, chanName, dbrType, value, dbrValue in self.channels:
            self.assertValueEqual(value, dbrValue.get())

    def tearDown(self):
        ca.sg_delete(self.gid)
        for chid, _, _, _, _ in self.channels:
            ca.clear_channel(chid)
        ca.flush_io()

if __name__ == '__main__':
    suit = unittest.TestSuite()
    suit.addTest(CaCreateTest("test_create", "catest"))
    suit.addTest(CaCreateTest("test_create_callback", "catest"))

    # catest is a record of single element DBF_DOUBLE
    # this tests the whole conversion matrix
    for dbfType in [ca.DBF_ENUM, ca.DBR_STRING, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG, ca.DBF_FLOAT, ca.DBF_DOUBLE]:
        for func in ['test_put', 'test_put_callback']:
            value = readback = 12.3
            if dbfType in [ca.DBF_ENUM, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG]:
                value = readback = 12
            if dbfType == ca.DBR_STRING:
                value = readback = '12.300' # well the precision is 3
            suit.addTest(CaPutTest(func, "catest",  dbfType,  1, value, readback))

    suit.addTest(CaPutTest('test_put', "catest",  ca.DBR_DOUBLE,  1, '12.3', 49.000)) # str converted to list [49,50,46,51]
    suit.addTest(CaPutTest('test_put', "catest",  ca.DBR_DOUBLE,  12, [12.3, 0, 0], 12.3)) # only the first element will be used
    # enum as string
    suit.addTest(CaPutTest('test_put', "cabo",    ca.DBR_STRING,  1, 'Busy'))
    # cawave is a record of 20 element DBF_DOUBLE
    suit.addTest(CaPutTest('test_put', "cawave",  ca.DBR_DOUBLE,  1, 1))
    suit.addTest(CaPutTest('test_put', "cawave",  ca.DBR_STRING,  3, ['1', '2', '3'], ['1.000', '2.000', '3.000']))
    suit.addTest(CaPutTest('test_put', "cawave",  ca.DBR_DOUBLE, 22, [0.0]*20))

    suit.addTest(CaPutTest('test_put', "cawaves", ca.DBR_STRING,  2, ['1', '2', '3'], ['1', '2']))

    suit.addTest(CaPutTest('test_put', "cawavec", ca.DBR_CHAR,    10, '123', [49, 50, 51, 0, 0]))

    # catest is a record of single element DBF_DOUBLE
    # this tests the whole conversion matrix
    for dbfType in [ca.DBF_ENUM, ca.DBR_STRING, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG, ca.DBF_FLOAT, ca.DBF_DOUBLE]:
        for func in ['test_get', 'test_get_callback', 'test_monitor']:
            value = 12.3
            if dbfType in [ca.DBF_ENUM, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG]:
                value = 12
            if dbfType == ca.DBR_STRING:
                value = '12.300' # well the precision is 3

            dbrType = ca.dbf_type_to_DBR(dbfType)
            suit.addTest(CaGetTest(func, "catest", dbrType, value))

            dbrType = ca.dbf_type_to_DBR_STS(dbfType)
            suit.addTest(CaGetTest(func, "catest", dbrType, value))

            dbrType = ca.dbf_type_to_DBR_TIME(dbfType)
            suit.addTest(CaGetTest(func, "catest", dbrType, value))

            dbrType = ca.dbf_type_to_DBR_GR(dbfType)
            suit.addTest(CaGetTest(func, "catest", dbrType, value))

            dbrType = ca.dbf_type_to_DBR_CTRL(dbfType)
            suit.addTest(CaGetTest(func, "catest", dbrType, value))

    # cabo is a record of DBF_ENUM
    # this tests the retrieval as number or string
    suit.addTest(CaGetTest("test_get", "cabo", ca.DBR_ENUM, 1))
    suit.addTest(CaGetTest("test_get", "cabo", ca.DBR_STRING, 'Busy'))
    suit.addTest(CaGetTest("test_get", "cabo", ca.DBR_GR_ENUM, 1))

    suit.addTest(CaGetTest("test_get_callback", "cabo", ca.DBR_ENUM, 1))
    suit.addTest(CaGetTest("test_get_callback", "cabo", ca.DBR_STRING, 'Busy'))
    suit.addTest(CaGetTest("test_get_callback", "cabo", ca.DBR_GR_ENUM, 1))

    suit.addTest(CaGetTest("test_monitor", "cabo", ca.DBR_ENUM, 1))
    suit.addTest(CaGetTest("test_monitor", "cabo", ca.DBR_STRING, 'Busy'))
    suit.addTest(CaGetTest("test_monitor", "cabo", ca.DBR_GR_ENUM, 1))

    # cawave is a record of 20 element DBF_DOUBLE
    # this tests the whole conversion matrix
    for dbfType in [ca.DBF_ENUM, ca.DBR_STRING, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG, ca.DBF_FLOAT, ca.DBF_DOUBLE]:
        for func in ['test_get', 'test_get_callback', 'test_monitor']:
            for use_numpy in [False, True]:
                value = [0.000] * 20
                if dbfType in [ca.DBF_ENUM, ca.DBF_CHAR, ca.DBF_SHORT, ca.DBF_LONG]:
                    value = [0] * 20
                if dbfType == ca.DBR_STRING:
                    value = ['0.000'] * 20 # well the precision is 3

                dbrType = ca.dbf_type_to_DBR(dbfType)
                suit.addTest(CaGetTest(func, "cawave", dbrType, value, use_numpy))

                dbrType = ca.dbf_type_to_DBR_STS(dbfType)
                suit.addTest(CaGetTest(func, "cawave", dbrType, value, use_numpy))

                dbrType = ca.dbf_type_to_DBR_TIME(dbfType)
                suit.addTest(CaGetTest(func, "cawave", dbrType, value, use_numpy))

                dbrType = ca.dbf_type_to_DBR_GR(dbfType)
                suit.addTest(CaGetTest(func, "cawave", dbrType, value, use_numpy))

                dbrType = ca.dbf_type_to_DBR_CTRL(dbfType)
                suit.addTest(CaGetTest(func, "cawave", dbrType, value, use_numpy))


    suit.addTest(CaGroupTest("test_group", [
                                ("cabo",   ca.DBR_STRING, "Busy"),
                                ('catest', ca.DBR_DOUBLE, 1),
                            ])
                 )


    runner = unittest.TextTestRunner()
    runner.run(suit)
