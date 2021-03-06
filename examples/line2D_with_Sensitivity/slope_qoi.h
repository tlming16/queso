#ifndef QUESO_EXAMPLE_SLOPE_QOI_H
#define QUESO_EXAMPLE_SLOPE_QOI_H

#include <queso/VectorFunction.h>
#include <queso/DistArray.h>

template<class P_V = QUESO::GslVector, class P_M = QUESO::GslMatrix,
	 class Q_V = QUESO::GslVector, class Q_M = QUESO::GslMatrix>
class Qoi : public QUESO::BaseVectorFunction<P_V, P_M, Q_V, Q_M>
{
public:
	Qoi(const char * prefix, const QUESO::VectorSet<P_V, P_M> & domainSet,
	    const QUESO::VectorSet<Q_V, Q_M> & imageSet);
	virtual ~Qoi();
	virtual void compute(const P_V & domainVector, const P_V * domainDirection,
		Q_V & imageVector, QUESO::DistArray<P_V *> * gradVectors,
		QUESO::DistArray<P_M *> * hessianMatrices,
		QUESO::DistArray<P_V *> * hessianEffects) const;

private:
	double x_loc;

};

#endif
	
