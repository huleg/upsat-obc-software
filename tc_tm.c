#include "tc_tm.h"

uint8_t checkSum( uint8_t *data, uint16_t size) {
  uint8_t CRC = 0;
  
  for(int i=0; i<=size; i++){
    CRC = CRC ^ data[i];
  }
  
  return CRC;
}

/*Must check for endianess*/
uint8_t unpack_pkt(const uint8_t *buf, struct tc_tm_pkt *pkt, const uint16_t size) {

	union _cnv cnv;
	uint8_t tmp_crc[2];

	uint8_t ver, dfield_hdr, ccsds_sec_hdr, tc_pus;


	tmp_crc[0] = buf[size - 1];
	tmp_crc[1] = checkSum( buf,size-2);

	if(tmp_crc[0] != tmp_crc[1]) {
		return R_ERROR;
	}

	ver = buf[0] >> 5;

	if(ver != 0) {
		return R_ERROR;
	}

	pkt->type = (buf[0] >> 4) & 0x01;
	dfield_hdr = (buf[0] >> 3) & 0x01;

    if(pkt->type != TC && pkt->type != TM) {
        return R_ERROR;
    }
    
	if(dfield_hdr != 1) {
		return R_ERROR;
	}

    cnv.cnv8[0] = buf[1];
    cnv.cnv8[1] = 0x07 & buf[0];
	pkt->app_id = cnv.cnv16[0];

	pkt->seq_flags = buf[2] >> 6;

	cnv.cnv8[0] = buf[3];
	cnv.cnv8[1] = buf[2] & 0x3F;
	pkt->seq_count = cnv.cnv16[0];

	cnv.cnv8[0] = buf[4];
	cnv.cnv8[1] = buf[5];
	pkt->len = cnv.cnv16[0];

	if ( pkt->len != size - 7 ) {
		return R_ERROR;
	}

    
    ccsds_sec_hdr = buf[6] >> 7;
    
    if(ccsds_sec_hdr != 0) {
        return R_ERROR;
    }
    
    tc_pus = buf[6] >> 4;

    if(tc_pus != 1) {
        return R_ERROR;
    }
    
    pkt->ack = 0x04 & buf[6];
    
    pkt->ser_type = buf[7];
    pkt->ser_subtype = buf[8];
    pkt->dest_id = buf[9];
    
    for(int i = 0; i < pkt->len-4; i++) {
 		pkt->data[i] = buf[10+i];
    }

    return R_OK;
}


uint8_t pack_pkt(uint8_t *buf, struct tc_tm_pkt *pkt, const uint16_t size) {

	union _cnv cnv;
	uint8_t buf_pointer;

	cnv.cnv16[0] = pkt->app_id;

	buf[0] = ( ECSS_VER_NUMBER << 5 | pkt->type << 4 | ECSS_DATA_FIELD_HDR_FLG << 3 | cnv.cnv8[1]);
	buf[1] = cnv.cnv8[0];

	cnv.cnv16[0] = pkt->seq_count;
	buf[2] = (  pkt->seq_flags << 6 | cnv.cnv8[1]);
	buf[3] = cnv.cnv8[0];

	/* TYPE = 0 TM, TYPE = 1 TC*/
	if(pkt->type == TM) {
		buf[6] = ECSS_PUS_VER << 4 ;
	} else if(pkt->type == TC) {
		buf[6] = ( ECSS_SEC_HDR_FIELD_FLG << 7 | ECSS_PUS_VER << 4 | pkt->ack);
	} else {
		return R_ERROR;
	}


	buf[7] = pkt->ser_type;
	buf[8] = pkt->ser_subtype;
	buf[9] = pkt->dest_id; /*source or destination*/

	if(pkt->ser_type == TC_VERIFICATION_SERVICE) {
		// cnv.cnv16[0] = tc_pkt_id;
		// cnv.cnv16[1] = tc_pkt_seq_ctrl;

		// buf[10] = cnv.cnv8[1];
		// buf[11] = cnv.cnv8[0];
		// buf[12] = cnv.cnv8[3];
		// buf[13] = cnv.cnv8[2];

		// if(ser_subtype == ACCEPTANCE_REPORT || ser_subtype == EXECUTION_REPORT || ser_subtype == COMPLETION_REPORT ) {
		// 	buf_pointer = 14;
		// } else if(ser_subtype == ACCEPTANCE_REPORT_FAIL || ser_subtype == EXECUTION_REPORT_FAIL|| ser_subtype == COMPLETION_REPORT_FAIL) {
		// 	buf[14] = code; 
		// 	buf_pointer = 15;
		// } else {
		// 	return R_ERROR;
		// }
	} else if(pkt->ser_type == TC_HOUSEKEEPING_SERVICE ) {

		// buf[10] = sid;
		// if(ser_subtype == 21 ) {
		// 	buf_pointer = 11;
		// } else if(ser_subtype == 23) {

		// 	if( sid == 3) {
		// 		cnv.cnv32 = time.now();
		// 		buf[12] = cnv.cnv8[3];
		// 		buf[13] = cnv.cnv8[2];
		// 		buf[14] = cnv.cnv8[1];
		// 		buf[15] = cnv.cnv8[0];
		// 	}
		// 	buf_pointer = 16;
		// } else {
		// 	return R_ERROR;
		// }
		

	} else if(pkt->ser_type == TC_FUNCTION_MANAGEMENT_SERVICE &&  pkt->ser_subtype == 1) {

		buf[10] = pkt->data[0];

		buf[11] = pkt->data[1];
		buf[12] = pkt->data[2];
		buf[13] = pkt->data[3];
		buf[14] = pkt->data[4];

		buf_pointer = 15;

	} else {
		return R_ERROR;
	}

	/*check if this is correct*/
	cnv.cnv16[0] = buf_pointer - 6;
	buf[4] = cnv.cnv8[0];
	buf[5] = cnv.cnv8[1];

	buf[buf_pointer++] = checkSum(buf, size-2);

    return R_OK;
}


// void route_pkt( uint8_t * pkt) {
// 	uint16_t id;

// 	if(type == TC) {
// 		id = app_id;
// 	} else if(type == TM) {
// 		id = destination_id;
// 	} else {
// 		/*return error*/
// 	}

// 	if(id == OBC) {

// 	} else if(id == EPS) {

// 	} else if(id == ADCS) {

// 	} else if(id == COMMS) {

// 	} else if(id == IAC) {

// 	} else {
// 	/*return error*/
// 	}
// }

