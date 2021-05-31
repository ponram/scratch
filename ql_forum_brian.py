import math
import datetime

mydate = Date(8, 10, 2014)
issueDate = Date(8, August, 2014)
maturityDate = Date(8, August, 2019)

Settings.instance().evaluationDate = mydate

spots = [mydate + Period("6m"), mydate + Period("1y"), mydate +
Period("2y"), mydate + Period("5y")]
spotsdate = [0.002, 0.002, 0.002, 0.002]
curveHandle = YieldTermStructureHandle(ZeroCurve(spots, spotsdate,
Actual360(), TARGET(), Linear(), Compounded, Semiannual))
myindex = Euribor6M(curveHandle)
myindex.addFixing(Date(6, August, 2014), spotsdate[1], True)

bond_schedule = Schedule(issueDate, maturityDate, Period(6, Months),
TARGET(), Following, Following, DateGeneration.Backward, False)

mybond = FloatingRateBond(3, 100, bond_schedule, myindex, Actual360())

mybond.setPricingEngine(DiscountingBondEngine(curveHandle))
mybond.NPV()
