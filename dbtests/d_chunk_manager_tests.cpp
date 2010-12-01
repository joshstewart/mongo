//@file d_chunk_manager_tests.cpp : s/d_chunk_manager.{h,cpp} tests

/**
*    Copyright (C) 2010 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pch.h"
#include "dbtests.h"

#include "../s/d_chunk_manager.h"

namespace {

    class BasicTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "test.foo" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 ) <<
                                       "unique"  << false );

            // single-chunk collection
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "test.foo-a_MinKey" << 
                                                 "ns"  << "test.foo" << 
                                                 "min" << BSON( "a" << MINKEY ) <<
                                                 "max" << BSON( "a" << MAXKEY ) ) );

            ShardChunkManager s ( collection , chunks );
            
            BSONObj k1 = BSON( "a" << MINKEY );
            ASSERT( s.belongsToMe( k1 ) );
            BSONObj k2 = BSON( "a" << MAXKEY );
            ASSERT( ! s.belongsToMe( k2 ) );
            BSONObj k3 = BSON( "a" << 1 << "b" << 2 );
            ASSERT( s.belongsToMe( k3 ) );
        }
    };

    class BasicCompoundTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "test.foo" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 << "b" << 1) <<
                                       "unique"  << false );

            // single-chunk collection
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "test.foo-a_MinKeyb_MinKey" << 
                                                 "ns"  << "test.foo" << 
                                                 "min" << BSON( "a" << MINKEY << "b" << MINKEY ) <<
                                                 "max" << BSON( "a" << MAXKEY << "b" << MAXKEY ) ) );

            ShardChunkManager s ( collection , chunks );
            
            BSONObj k1 = BSON( "a" << MINKEY << "b" << MINKEY );
            ASSERT( s.belongsToMe( k1 ) );
            BSONObj k2 = BSON( "a" << MAXKEY << "b" << MAXKEY );
            ASSERT( ! s.belongsToMe( k2 ) );
            BSONObj k3 = BSON( "a" << MINKEY << "b" << 10 );
            ASSERT( s.belongsToMe( k3 ) );
            BSONObj k4 = BSON( "a" << 10 << "b" << 20 );
            ASSERT( s.belongsToMe( k4 ) );            
        }
    };

    class RangeTests { 
    public:
        void run() { 
            BSONObj collection = BSON( "_id"     << "x.y" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 ) <<
                                       "unique"  << false );

            // 3-chunk collection, 2 of them being contiguous
            // [min->10) , [10->20) , <gap> , [30->max)
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "x.y-a_MinKey" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << MINKEY ) << 
                                                 "max" << BSON( "a" << 10 ) ) <<
                                           BSON( "_id" << "x.y-a_10" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 10 ) << 
                                                 "max" << BSON( "a" << 20 ) ) <<
                                           BSON( "_id" << "x.y-a_30" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 30 ) << 
                                                 "max" << BSON( "a" << MAXKEY ) ) );

            ShardChunkManager s ( collection , chunks );

            BSONObj k1 = BSON( "a" << 5 );
            ASSERT( s.belongsToMe( k1 ) );            
            BSONObj k2 = BSON( "a" << 10 );
            ASSERT( s.belongsToMe( k2 ) );
            BSONObj k3 = BSON( "a" << 25 );
            ASSERT( ! s.belongsToMe( k3 ) );
            BSONObj k4 = BSON( "a" << 30 );
            ASSERT( s.belongsToMe( k4 ) );
            BSONObj k5 = BSON( "a" << 40 );
            ASSERT( s.belongsToMe( k5 ) );
        }
    };

    class DeletedTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "test.foo" <<
                                       "dropped" << "true" );

            BSONArray chunks = BSONArray();

            ASSERT_EXCEPTION( ShardChunkManager s ( collection , chunks ) , UserException );
        }
    };

    class ClonePlusTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "test.foo" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 << "b" << 1 ) <<
                                       "unique"  << false );
            // 1-chunk collection
            // [10,0-20,0)
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "test.foo-a_MinKey" << 
                                                 "ns"  << "test.foo" << 
                                                 "min" << BSON( "a" << 10 << "b" << 0 ) <<
                                                 "max" << BSON( "a" << 20 << "b" << 0 ) ) );

            ShardChunkManager s ( collection , chunks );

            // new chunk [20,0-30,0)
            BSONObj min = BSON( "a" << 20 << "b" << 0 );
            BSONObj max = BSON( "a" << 30 << "b" << 0 );
            ShardChunkManagerPtr cloned( s.clonePlus( min , max , 1 /* TODO test version */ ) );

            BSONObj k1 = BSON( "a" << 5 << "b" << 0 );
            ASSERT( ! cloned->belongsToMe( k1 ) );                        
            BSONObj k2 = BSON( "a" << 20 << "b" << 0 );
            ASSERT( cloned->belongsToMe( k2 ) );            
            BSONObj k3 = BSON( "a" << 25 << "b" << 0 );
            ASSERT( cloned->belongsToMe( k3 ) );            
            BSONObj k4 = BSON( "a" << 30 << "b" << 0 );
            ASSERT( ! cloned->belongsToMe( k4 ) );            
        }
    };

    class ClonePlusExceptionTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "test.foo" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 << "b" << 1 ) <<
                                       "unique"  << false );
            // 1-chunk collection
            // [10,0-20,0)
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "test.foo-a_MinKey" << 
                                                 "ns"  << "test.foo" << 
                                                 "min" << BSON( "a" << 10 << "b" << 0 ) <<
                                                 "max" << BSON( "a" << 20 << "b" << 0 ) ) );

            ShardChunkManager s ( collection , chunks );

            // [15,0-25,0) overlaps [10,0-20,0)
            BSONObj min = BSON( "a" << 15 << "b" << 0 );
            BSONObj max = BSON( "a" << 25 << "b" << 0 );
            ASSERT_EXCEPTION( s.clonePlus ( min , max , 1 /* TODO test version */ ) , UserException );
        }
    };

    class CloneMinusTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "x.y" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 << "b" << 1 ) << 
                                       "unique"  << false );

            // 2-chunk collection
            // [10,0->20,0) , <gap> , [30,0->40,0)
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "x.y-a_10b_0" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 10 << "b" << 0 ) << 
                                                 "max" << BSON( "a" << 20 << "b" << 0 ) ) <<
                                           BSON( "_id" << "x.y-a_30b_0" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 30 << "b" << 0 ) << 
                                                 "max" << BSON( "a" << 40 << "b" << 0 ) ) );

            ShardChunkManager s ( collection , chunks );

            // deleting chunk [10,0-20,0)
            BSONObj min = BSON( "a" << 10 << "b" << 0 );
            BSONObj max = BSON( "a" << 20 << "b" << 0 );
            ShardChunkManagerPtr cloned( s.cloneMinus( min , max , 1 /* TODO test version */ ) );

            BSONObj k1 = BSON( "a" << 5 << "b" << 0 );
            ASSERT( ! cloned->belongsToMe( k1 ) );                        
            BSONObj k2 = BSON( "a" << 15 << "b" << 0 );
            ASSERT( ! cloned->belongsToMe( k2 ) );            
            BSONObj k3 = BSON( "a" << 30 << "b" << 0 );
            ASSERT( cloned->belongsToMe( k3 ) );
            BSONObj k4 = BSON( "a" << 35 << "b" << 0 );
            ASSERT( cloned->belongsToMe( k4 ) );
            BSONObj k5 = BSON( "a" << 40 << "b" << 0 );
            ASSERT( ! cloned->belongsToMe( k5 ) );
        }
    };

    class CloneMinusExceptionTests {
    public:
        void run() {
            BSONObj collection = BSON( "_id"     << "x.y" <<
                                       "dropped" << false <<
                                       "key"     << BSON( "a" << 1 << "b" << 1 ) << 
                                       "unique"  << false );

            // 2-chunk collection
            // [10->20) , <gap> , [30->40)
            BSONArray chunks = BSON_ARRAY( BSON( "_id" << "x.y-a_10b_0" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 10 << "b" << 0 ) << 
                                                 "max" << BSON( "a" << 20 << "b" << 0 ) ) <<
                                           BSON( "_id" << "x.y-a_30b_0" << 
                                                 "ns"  << "x.y" << 
                                                 "min" << BSON( "a" << 30 << "b" << 0 ) << 
                                                 "max" << BSON( "a" << 40 << "b" << 0 ) ) );

            ShardChunkManager s ( collection , chunks );

            // deleting non-existing chunk [25,0-28,0)
            BSONObj min1 = BSON( "a" << 25 << "b" << 0 );
            BSONObj max1 = BSON( "a" << 28 << "b" << 0 );
            ASSERT_EXCEPTION( s.cloneMinus( min1 , max1 , 1 /* TODO test version */ ) , UserException );


            // deletin an overlapping range (not exactly a chunk) [15,0-25,0)
            BSONObj min2 = BSON( "a" << 15 << "b" << 0 );
            BSONObj max2 = BSON( "a" << 25 << "b" << 0 );
            ASSERT_EXCEPTION( s.cloneMinus( min2 , max2 , 1 /* TODO test version */ ) , UserException );
        }
    };

    class ShardChunkManagerSuite : public Suite {
    public:
        ShardChunkManagerSuite() : Suite ( "shard_chunk_manager" ) {}

        void setupTests() {
            add< BasicTests >();
            add< BasicCompoundTests >();
            add< RangeTests >();
            add< DeletedTests >();
            add< ClonePlusTests >();
            add< ClonePlusExceptionTests >();
            add< CloneMinusTests >();
            add< CloneMinusExceptionTests >();
        }
    } shardChunkManagerSuite;

}  // anonymous namespace