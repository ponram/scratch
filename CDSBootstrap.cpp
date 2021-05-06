#include <vector>
#include <iostream>
#include <ql/quantlib.hpp>

using namespace std;
using namespace QuantLib;

/*	--------------------------------------------------------------------------------
	-- Mock external interface. Deliberate misspelling to avoid name clashes.
	-------------------------------------------------------------------------------- */
struct Deit
{
	int day;
	int month;
	int year;
};

// -- Term Structure Node
struct TSNode
{
	Deit date;
	double tenor;
	double sp;		// survival probability
	double hr;		// hazard rate
};

/*	--------------------------------------------------------------------------------
	-- main()
	-------------------------------------------------------------------------------- */
int main()
{
	try
	{	
		/*	--------------------------------------------------------------------------------
			-- DATA
			-------------------------------------------------------------------------------- */
		Deit tradeDeit = { 13, 6, 2011 };
		
		int settlementDays = 1;

		auto isdaSwapDeits = std::vector<Deit>{
			tradeDeit,
			Deit{15, 7, 2011}, // July 15th, 2011,
			Deit{15, 8, 2011}, // August 15th, 2011,
			Deit{15, 9, 2011}, // September 15th, 2011
			Deit{15, 12, 2011}, // December 15th, 2011
			Deit{15, 3, 2012}, // March 15th, 2012
			Deit{15, 6, 2012}, // June 15th, 2012
			Deit{17, 6, 2013}, // June 17th, 2013
			Deit{16, 6, 2014}, // June 16th, 2014
			Deit{15, 6, 2015}, // June 15th, 2015
			Deit{15, 6, 2016}, // June 15th, 2016
			Deit{15, 6, 2017}, // June 15th, 2017
			Deit{15, 6, 2018}, // June 15th, 2018
			Deit{17, 6, 2019}, // June 17th, 2019
			Deit{15, 6, 2020}, // June 15th, 2020
			Deit{15, 6, 2021}, // June 15th, 2021
			Deit{15, 6, 2022}, // June 15th, 2022
			Deit{15, 6, 2023}, // June 15th, 2023
			Deit{15, 6, 2026}, // June 15th, 2026
			Deit{16, 6, 2031}, // June 16th, 2031
			Deit{16, 6, 2036}, // June 16th, 2036
			Deit{17, 6, 2041}  // June 17th, 2041
		};

		auto isdaSwapDiscountFactors = std::vector<double>{
			1.00,
			0.999901,
			0.999771,
			0.999488,
			0.99897,
			0.997154,
			0.994633,
			0.986013,
			0.970953,
			0.951254,
			0.929146,
			0.905782,
			0.882807,
			0.859751,
			0.837656,
			0.792561,
			0.732218,
			0.64638,
			0.572974,
			0.512867,
			0.4512867,
			0.3512867
		};

		auto cdsParSpreads = std::vector<double>{
			0.007927,
			0.007927,
			0.012239,
			0.016979,
			0.019271,
			0.020860
		};

		// -- All tenors in years.
		auto cdsTenors = std::vector<double>{
			0.5,
			1.0,
			3.0,
			5.0,
			7.0,
			10.0
		};

		auto cdsRecoveryRates = std::vector<double>{
			0.4,
			0.4,
			0.4,
			0.4,
			0.4,
			0.4
		};
		
		/*	--------------------------------------------------------------------------------
			-- CALCULATION
			-------------------------------------------------------------------------------- */
		Calendar calendar = WeekendsOnly();

		Date tradeDate = Date(tradeDeit.day, Month(tradeDeit.month), tradeDeit.year);

		Settings::instance().evaluationDate() = tradeDate;

		auto nisdaSwapDeits = isdaSwapDeits.size();
		std::vector<Date> isdaSwapDates(nisdaSwapDeits);

		for (int k = 0; k < nisdaSwapDeits; ++k) {
			isdaSwapDates[k] = Date(isdaSwapDeits[k].day, Month(isdaSwapDeits[k].month), isdaSwapDeits[k].year);
		}

		// -- CONSTRUCT THE YIELD CURVE
		Handle<YieldTermStructure> isdaSwapDiscountCurve = Handle<YieldTermStructure>(
			ext::make_shared<DiscountCurve>(
				isdaSwapDates,
				isdaSwapDiscountFactors,
				Actual365Fixed(),
				LogLinear()
				)
			);
		isdaSwapDiscountCurve->enableExtrapolation();

		CreditDefaultSwap::PricingModel model = CreditDefaultSwap::ISDA;

		int nParSpreads = cdsParSpreads.size();
		std::vector<ext::shared_ptr<DefaultProbabilityHelper> > isdaCdsHelpers(nParSpreads);

		// -- Calibration instruments
		for (int k = 0; k < nParSpreads; ++k) {
			isdaCdsHelpers[k] = ext::shared_ptr<CdsHelper>(
				new SpreadCdsHelper(
					cdsParSpreads[k],
					// -- Years to months
					cdsTenors[k] * 12.0 * Months,
					settlementDays,
					calendar,
					Quarterly,
					Following,
					DateGeneration::TwentiethIMM,
					Actual360(),
					cdsRecoveryRates[k],
					isdaSwapDiscountCurve,
					true,
					true,
					Date(),
					Actual360(true),
					true,
					model
				)
				);
		}

		// -- CONSTRUCT THE CREDIT CURVE
		// -- The follwing fails if Actual360() is used...
		Handle<DefaultProbabilityTermStructure> isdaCts = Handle<DefaultProbabilityTermStructure>(
			ext::make_shared<PiecewiseDefaultCurve<SurvivalProbability, LogLinear> >(0, calendar, isdaCdsHelpers, Actual365Fixed()));

		// -- Term structure of survival probabilities and hazard rates.
		std::vector<TSNode> SurvProbTermStruct(nParSpreads);
		TSNode tsnode;

		for (int k = 0; k < isdaCdsHelpers.size(); k++) {
			Date d = isdaCdsHelpers[k]->latestDate();
			double t = isdaCts->timeFromReference(d);
			tsnode.date.year = d.year();
			tsnode.date.month = int(d.month());
			tsnode.date.day = d.dayOfMonth();
			tsnode.tenor = t;
			tsnode.sp = isdaCts->survivalProbability(d);
			tsnode.hr = isdaCts->hazardRate(t);
			SurvProbTermStruct[k] = tsnode;
		}

		// --------------------------------------------------------------------------------
		// --> SURPRISE!
		// --------------------------------------------------------------------------------
		// Re-price calibration instruments
		double notional = 1000000.0;
		
		// -- Gives an NPV of ~ $22
		//ext::shared_ptr<PricingEngine> engine(new IsdaCdsEngine(isdaCts, 0.4, isdaSwapDiscountCurve));

		// -- Gives an NPV of ~ 17 cents
		ext::shared_ptr<PricingEngine> engine(new MidPointCdsEngine(isdaCts, 0.4, isdaSwapDiscountCurve));

		Date settlementDate = calendar.advance(tradeDate, settlementDays, Days);
		Date terminationDate = calendar.adjust(settlementDate + cdsTenors[0] * 12 * Months, Following);

		Schedule cdsSchedule = MakeSchedule()
			.from(settlementDate)
			.to(terminationDate)
			.withFrequency(Quarterly)
			.withCalendar(calendar)
			.withTerminationDateConvention(Unadjusted)
			.withRule(DateGeneration::TwentiethIMM);
		CreditDefaultSwap cds(Protection::Seller, notional, cdsParSpreads[0], cdsSchedule, Following, Actual360());
		cds.setPricingEngine(engine);

		cout << "-- Repricing the first instrument used for calibration: " << endl;
		cout << "-- par spread: " << io::rate(cds.fairSpread()) << endl
			<< "    NPV:         " << cds.NPV() << endl
			<< "    default leg: " << cds.defaultLegNPV() << endl
			<< "    coupon leg:  " << cds.couponLegNPV() << endl << endl;
		// --------------------------------------------------------------------------------
		// SURPRISE! <--
		// --------------------------------------------------------------------------------

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "** Exception raised:" << std::endl;
		std::cerr << e.what() << std::endl;
		return 42;
	}

}

