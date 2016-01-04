/** @file triphistory_cmd.cpp */

#include "stdafx.h"
#include "triphistory.h"
#include "table/strings.h"

void
TripHistory::AddValue( Money mvalue, Date dvalue ) {
	if ( 0 < dvalue ) {
		t[ 0 ].profit += mvalue;
		t[ 0 ].date = dvalue;
	}
}

void
TripHistory::NewRound( ) {
	//move down
	for ( int i = TRIP_LENGTH - 1; i > 0; i-- ) {
		this->t[ i ] = this->t[ i - 1 ];
		//this->trip_history_date_array[ i ] = this->trip_history_date_array[ i - 1 ];
	}

	this->t[ 0 ].profit = 0;
	this->t[ 0 ].date = this->t[ 1 ].date;

	//t.push_front( TripHistoryEntry( ) );
}

size_t
TripHistory::UpdateCalculated( ) {

	this->total_profit = 0;
	this->total_change = 0;
	this->avg_daylength = 0;
	this->profit_per_day = 0;
	uint i = 0;

	//
	while ( i < TRIP_LENGTH && t [ i ].date ) {
		
		if ( i > 0 ) {
			t[ i - 1 ].profit_change =
				FindPercentChange( t [ i - 1 ].profit, t[ i ].profit );
			t[ i - 1 ].TBT = t[ i - 1 ].date - t[ i ].date;

			if ( i > 1 ) t[ i - 2 ].TBT_change = t[ i - 2 ].TBT - t[ i - 1 ].TBT;//bad line i don't like it

			//omit first -100% row
			if ( i > 1 || t [ 0 ].profit_change != -100 )
				this->total_change += t[ i - 1 ].profit_change;
			this->avg_daylength += t[ i - 1 ].TBT;
		}

		// prepare summary
		

		this->total_profit += t[ i ].profit;
		i++;
	}

	if ( i == 0 ) return 0 ;
	
	this->avg_daylength /= --i + 1;

	if ( t[ 0 ].date != t[ i ].date ) {
		this->profit_per_day = total_profit / ( t[ 0 ].date - t[ i ].date );
	}

	return i;
	/*
	Trips::reverse_iterator i = t.rbegin( );
	while ( i < t.rend( ) ) {

		if ( i + 1 != t.rend( ) ) {
			(*( i + 1 )).profit_change =
				FindPercentChange( ( *i ).profit, ( *( i + 1 ) ).profit ); // reverse_itenrator

			( *( i + 1 ) ).TBT = ( *i ).date - ( *( i + 1 ) ).date;

			if ( ( *i ).TBT ) ( *( i + 1 ) ).TBT_change = ( *i ).TBT - ( *( i + 1 ) ).TBT;

			//omit first -100% row
			this->total_change += ( *i ).profit_change;
		}

		// prepare summary
		this->total_profit += ( *i ).profit;
		this->avg_daylength += ( *i ).TBT;

		i++;
	}

	this->avg_daylength /= t.size( );

	if ( t.front( ).date != t.back( ).date ) {
		this->profit_per_day = total_profit / ( t.front( ).date - t.back( ).date );
	}

	return t.size( );*/
}
