/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTW_DEBUG_C_


#include <rtw_debug.h>
#include <../hal/dm.h>

//#ifdef CONFIG_DEBUG_RTL871X

	u32 GlobalDebugLevel = _drv_err_;

	u64 GlobalDebugComponents = \
			_module_rtl871x_xmit_c_ |
			_module_xmit_osdep_c_ |
			_module_rtl871x_recv_c_ |
			_module_recv_osdep_c_ |
			_module_rtl871x_mlme_c_ |
			_module_mlme_osdep_c_ |
			_module_rtl871x_sta_mgt_c_ |
			_module_rtl871x_cmd_c_ |
			_module_cmd_osdep_c_ |
			_module_rtl871x_io_c_ |
			_module_io_osdep_c_ |
			_module_os_intfs_c_|
			_module_rtl871x_security_c_|
			_module_rtl871x_eeprom_c_|
			_module_hal_init_c_|
			_module_hci_hal_init_c_|
			_module_rtl871x_ioctl_c_|
			_module_rtl871x_ioctl_set_c_|
			_module_rtl871x_ioctl_query_c_|
			_module_rtl871x_pwrctrl_c_|
			_module_hci_intfs_c_|
			_module_hci_ops_c_|
			_module_hci_ops_os_c_|
			_module_rtl871x_ioctl_os_c|
			_module_rtl8712_cmd_c_|
			_module_hal_xmit_c_|
			_module_rtl8712_recv_c_ |
			_module_mp_ |
			_module_efuse_;

//#endif

#ifdef CONFIG_PROC_DEBUG
#include <rtw_version.h>

int proc_get_drv_version(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;

	len += snprintf(page + len, count - len, "%s\n", DRIVERVERSION);
				
	*eof = 1;
	return len;
}

int proc_get_log_level(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;

	len += snprintf(page + len, count - len,
		"log_level:%d\n",
		GlobalDebugLevel
		);

	*eof = 1;
	return len;
}

int proc_set_log_level(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	u32 is_signal_dbg;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d ", &is_signal_dbg);

		if( is_signal_dbg >= 0 && is_signal_dbg < 10 )
		{
			GlobalDebugLevel= is_signal_dbg;
			printk("%d\n", GlobalDebugLevel);
		}
	}
	
	return count;
	
}

#ifdef DBG_MEM_ALLOC
int proc_get_mstat(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{	
	int len = 0;

	len += _rtw_mstat_dump(page+len, count-len);
	*eof = 1;

	return len;
}
#endif /* DBG_MEM_ALLOC */

int proc_get_write_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	*eof = 1;
	return 0;
}

int proc_set_write_reg(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	u32 addr, val, len;

	if (count < 3)
	{
		DBG_871X("argument size is less than 3\n");
		return -EFAULT;
	}	

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%x %x %x", &addr, &val, &len);

		if (num !=  3) {
			DBG_871X("invalid write_reg parameter!\n");
			return count;
		}

		switch(len)
		{
			case 1:
				rtw_write8(padapter, addr, (u8)val);				
				break;
			case 2:
				rtw_write16(padapter, addr, (u16)val);				
				break;
			case 4:
				rtw_write32(padapter, addr, val);				
				break;
			default:
				DBG_871X("error write length=%d", len);
				break;
		}			
		
	}
	
	return count;
	
}

static u32 proc_get_read_addr=0xeeeeeeee;
static u32 proc_get_read_len=0x4;

int proc_get_read_reg(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{	
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	
	int len = 0;

	if(proc_get_read_addr==0xeeeeeeee)
	{
		*eof = 1;
		return len;
	}	

	switch(proc_get_read_len)
	{
		case 1:			
			len += snprintf(page + len, count - len, "rtw_read8(0x%x)=0x%x\n", proc_get_read_addr, rtw_read8(padapter, proc_get_read_addr));
			break;
		case 2:
			len += snprintf(page + len, count - len, "rtw_read16(0x%x)=0x%x\n", proc_get_read_addr, rtw_read16(padapter, proc_get_read_addr));
			break;
		case 4:
			len += snprintf(page + len, count - len, "rtw_read32(0x%x)=0x%x\n", proc_get_read_addr, rtw_read32(padapter, proc_get_read_addr));
			break;
		default:
			len += snprintf(page + len, count - len, "error read length=%d\n", proc_get_read_len);
			break;
	}

	*eof = 1;
	return len;

}

int proc_set_read_reg(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	char tmp[16];
	u32 addr, len;

	if (count < 2)
	{
		DBG_871X("argument size is less than 2\n");
		return -EFAULT;
	}	

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%x %x", &addr, &len);

		if (num !=  2) {
			DBG_871X("invalid read_reg parameter!\n");
			return count;
		}

		proc_get_read_addr = addr;
		
		proc_get_read_len = len;
	}
	
	return count;

}

int proc_get_fwstate(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;

	len += snprintf(page + len, count - len, "fwstate=0x%x\n", get_fwstate(pmlmepriv));
				
	*eof = 1;
	return len;
}

int proc_get_sec_info(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	
	int len = 0;

	len += snprintf(page + len, count - len, "auth_alg=0x%x, enc_alg=0x%x, auth_type=0x%x, enc_type=0x%x\n", 
						psecuritypriv->dot11AuthAlgrthm, psecuritypriv->dot11PrivacyAlgrthm,
						psecuritypriv->ndisauthtype, psecuritypriv->ndisencryptstatus);
				
	*eof = 1;
	return len;
}

int proc_get_mlmext_state(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	int len = 0;

	len += snprintf(page + len, count - len, "pmlmeinfo->state=0x%x\n", pmlmeinfo->state);
				
	*eof = 1;
	return len;
}

int proc_get_qos_option(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;

	len += snprintf(page + len, count - len, "qos_option=%d\n", pmlmepriv->qospriv.qos_option);
				
	*eof = 1;
	return len;

}

int proc_get_ht_option(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;

	len += snprintf(page + len, count - len, "ht_option=%d\n", pmlmepriv->htpriv.ht_option);
				
	*eof = 1;
	return len;
}

int proc_get_rf_info(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	int len = 0;

	len += snprintf(page + len, count - len, "cur_ch=%d, cur_bw=%d, cur_ch_offet=%d\n"
		"oper_ch=%d, oper_bw=%d, oper_ch_offet=%d\n",
		pmlmeext->cur_channel, pmlmeext->cur_bwmode, pmlmeext->cur_ch_offset,
		rtw_get_oper_ch(padapter), rtw_get_oper_bw(padapter), rtw_get_oper_choffset(padapter));
	*eof = 1;

	return len;
}

int proc_get_ap_info(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct sta_info *psta;
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct wlan_network *cur_network = &(pmlmepriv->cur_network);
	struct sta_priv *pstapriv = &padapter->stapriv;
	int len = 0;

	psta = rtw_get_stainfo(pstapriv, cur_network->network.MacAddress);
	if(psta)
	{
		int i;
		struct recv_reorder_ctrl *preorder_ctrl;
					
		len += snprintf(page + len, count - len, "SSID=%s\n", cur_network->network.Ssid.Ssid);		
		len += snprintf(page + len, count - len, "sta's macaddr:" MAC_FMT "\n", MAC_ARG(psta->hwaddr));
		len += snprintf(page + len, count - len, "cur_channel=%d, cur_bwmode=%d, cur_ch_offset=%d\n", pmlmeext->cur_channel, pmlmeext->cur_bwmode, pmlmeext->cur_ch_offset);		
		len += snprintf(page + len, count - len, "rtsen=%d, cts2slef=%d\n", psta->rtsen, psta->cts2self);
		len += snprintf(page + len, count - len, "qos_en=%d, ht_en=%d, init_rate=%d\n", psta->qos_option, psta->htpriv.ht_option, psta->init_rate);	
		len += snprintf(page + len, count - len, "state=0x%x, aid=%d, macid=%d, raid=%d\n", psta->state, psta->aid, psta->mac_id, psta->raid);	
		len += snprintf(page + len, count - len, "bwmode=%d, ch_offset=%d, sgi=%d\n", psta->htpriv.bwmode, psta->htpriv.ch_offset, psta->htpriv.sgi);						
		len += snprintf(page + len, count - len, "ampdu_enable = %d\n", psta->htpriv.ampdu_enable);	
		len += snprintf(page + len, count - len, "agg_enable_bitmap=%x, candidate_tid_bitmap=%x\n", psta->htpriv.agg_enable_bitmap, psta->htpriv.candidate_tid_bitmap);
					
		for(i=0;i<16;i++)
		{							
			preorder_ctrl = &psta->recvreorder_ctrl[i];
			if(preorder_ctrl->enable)
			{
				len += snprintf(page + len, count - len, "tid=%d, indicate_seq=%d\n", i, preorder_ctrl->indicate_seq);
			}
		}	
							
	}
	else
	{							
		len += snprintf(page + len, count - len, "can't get sta's macaddr, cur_network's macaddr:" MAC_FMT "\n", MAC_ARG(cur_network->network.MacAddress));
	}

	*eof = 1;
	return len;

}

int proc_get_adapter_state(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	
	len += snprintf(page + len, count - len, "bSurpriseRemoved=%d, bDriverStopped=%d\n", 
						padapter->bSurpriseRemoved, padapter->bDriverStopped);

	*eof = 1;
	return len;

}
	
int proc_get_trx_info(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	int i;
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct recv_priv  *precvpriv = &padapter->recvpriv;
	struct dvobj_priv	*pdvobj = adapter_to_dvobj(padapter);
        struct hw_xmit *phwxmit;
	int len = 0;
	
	len += snprintf(page + len, count - len, "free_xmitbuf_cnt=%d, free_xmitframe_cnt=%d"
				", free_ext_xmitbuf_cnt=%d, free_xframe_ext_cnt=%d"
				", free_recvframe_cnt=%d\n",
				pxmitpriv->free_xmitbuf_cnt, pxmitpriv->free_xmitframe_cnt,
				pxmitpriv->free_xmit_extbuf_cnt, pxmitpriv->free_xframe_ext_cnt,
				precvpriv->free_recvframe_cnt);
#ifdef CONFIG_USB_HCI
	len += snprintf(page + len, count - len, "rx_urb_pending_cnt=%d\n", precvpriv->rx_pending_cnt);
#endif

	len += snprintf(page + len, count - len, "recvbuf_skb_alloc_fail_cnt=%d\n", precvpriv->recvbuf_skb_alloc_fail_cnt);
	len += snprintf(page + len, count - len, "recvbuf_null_cnt=%d\n", precvpriv->recvbuf_null_cnt);
	len += snprintf(page + len, count - len, "read_port_complete_EINPROGRESS_cnt=%d\n", precvpriv->read_port_complete_EINPROGRESS_cnt);
	len += snprintf(page + len, count - len, "read_port_complete_other_urb_err_cnt=%d\n", precvpriv->read_port_complete_other_urb_err_cnt);
	len += snprintf(page + len, count - len, "hw_init_completed=%d\n", padapter->hw_init_completed);
#ifdef CONFIG_USB_HCI
	len += snprintf(page + len, count - len, "continual_urb_error=%d\n", atomic_read(&pdvobj->continual_urb_error));
#endif

        for(i = 0; i < 4; i++) 
	{
		phwxmit = pxmitpriv->hwxmits + i;
		len += snprintf(page + len, count - len, "%d, hwq.accnt=%d\n", i, phwxmit->accnt);
	}

	*eof = 1;
	return len;

}
	
		

int proc_get_mac_reg_dump1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1;

	len += snprintf(page + len, count - len, "\n======= MAC REG =======\n");

	for(i=0x0;i<0x300;i+=4)
	{	
		if(j%4==1)	len += snprintf(page + len, count - len,"0x%02x",i);
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}

	*eof = 1;
	return len;

}

int proc_get_mac_reg_dump2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1;

	len += snprintf(page + len, count - len, "\n======= MAC REG =======\n");
	memset(page, 0, count);
	for(i=0x300;i<0x600;i+=4)
	{	
		if(j%4==1)	len += snprintf(page + len, count - len,"0x%02x",i);
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}
	
	*eof = 1;
	return len;

}

int proc_get_mac_reg_dump3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1;

	len += snprintf(page + len, count - len, "\n======= MAC REG =======\n");

	for(i=0x600;i<0x800;i+=4)
	{	
		if(j%4==1)	len += snprintf(page + len, count - len,"0x%02x",i);
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}

	*eof = 1;
	return len;

}

int proc_get_bb_reg_dump1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	int len = 0;
	int i,j=1;		

	len += snprintf(page + len, count - len, "\n======= BB REG =======\n");	
	for(i=0x800;i<0xB00;i+=4)
	{
		if(j%4==1) 	len += snprintf(page + len, count - len,"0x%02x",i);		
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}
	*eof = 1;
	return len;	
}

int proc_get_bb_reg_dump2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	int len = 0;
	int i,j=1;		

	len += snprintf(page + len, count - len, "\n======= BB REG =======\n");	
	for(i=0xB00;i<0xE00;i+=4)
	{
		if(j%4==1) 	len += snprintf(page + len, count - len,"0x%02x",i);		
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}
	*eof = 1;
	return len;	
}

int proc_get_bb_reg_dump3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);	
	int len = 0;
	int i,j=1;		

	len += snprintf(page + len, count - len, "\n======= BB REG =======\n");	
	for(i=0xE00;i<0x1000;i+=4)
	{
		if(j%4==1) 	len += snprintf(page + len, count - len,"0x%02x",i);		
		len += snprintf(page + len, count - len," 0x%08x ",rtw_read32(padapter,i));		
		if((j++)%4 == 0)	len += snprintf(page + len, count - len,"\n");	
	}
	*eof = 1;
	return len;	
}

int proc_get_rf_reg_dump1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1,path;
	u32 value;
	
	len += snprintf(page + len, count - len, "\n======= RF REG =======\n");
	path = 1;
	len += snprintf(page + len, count - len, "\nRF_Path(%x)\n",path);
	for(i=0;i<0xC0;i++)
	{								
		//value = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)path,i, bMaskDWord);
		value =rtw_hal_read_rfreg(padapter, path, i, 0xffffffff);
		if(j%4==1)	len += snprintf(page + len, count - len, "0x%02x ",i);
		len += snprintf(page + len, count - len, " 0x%08x ",value);
		if((j++)%4==0)	len += snprintf(page + len, count - len, "\n");	
	}

	*eof = 1;
	return len;	
}


int proc_get_rf_reg_dump2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1,path;
	u32 value;	

	len += snprintf(page + len, count - len, "\n======= RF REG =======\n");	
	path = 1;
	len += snprintf(page + len, count - len, "\nRF_Path(%x)\n",path);
	for(i=0xC0;i<0x100;i++)
	{								
		//value = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)path,i, bMaskDWord);
		value =rtw_hal_read_rfreg(padapter, path, i, 0xffffffff);
		if(j%4==1)	len += snprintf(page + len, count - len, "0x%02x ",i);
		len += snprintf(page + len, count - len, " 0x%08x ",value);
		if((j++)%4==0)	len += snprintf(page + len, count - len, "\n");	
	}
	*eof = 1;
	return len;	
}


int proc_get_rf_reg_dump3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1,path;
	u32 value;	

	len += snprintf(page + len, count - len, "\n======= RF REG =======\n");
	path = 2;
	len += snprintf(page + len, count - len, "\nRF_Path(%x)\n",path);
	for(i=0;i<0xC0;i++)
	{								
		//value = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)path,i, bMaskDWord);
		value =rtw_hal_read_rfreg(padapter, path, i, 0xffffffff);
		if(j%4==1)	len += snprintf(page + len, count - len, "0x%02x ",i);
		len += snprintf(page + len, count - len, " 0x%08x ",value);
		if((j++)%4==0)	len += snprintf(page + len, count - len, "\n");	
	}

	*eof = 1;
	return len;	
}


int proc_get_rf_reg_dump4(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;
	int i,j=1,path;
	u32 value;

	len += snprintf(page + len, count - len, "\n======= RF REG =======\n");
	path = 2;
	len += snprintf(page + len, count - len, "\nRF_Path(%x)\n",path);
	for(i=0xC0;i<0x100;i++)
	{								
		//value = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)path,i, bMaskDWord);
		value =rtw_hal_read_rfreg(padapter, path, i, 0xffffffff);
		if(j%4==1)	len += snprintf(page + len, count - len, "0x%02x ",i);
		len += snprintf(page + len, count - len, " 0x%08x ",value);
		if((j++)%4==0)	len += snprintf(page + len, count - len, "\n");
	}
	*eof = 1;
	return len;	
}


		
int proc_get_rx_signal(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;

	len += snprintf(page + len, count - len,
		"rssi:%d\n"
		"rxpwdb:%d\n"
		"signal_strength:%u\n"
		"signal_qual:%u\n"
		"noise:%u\n", 
		padapter->recvpriv.rssi,
		padapter->recvpriv.rxpwdb,
		padapter->recvpriv.signal_strength,
		padapter->recvpriv.signal_qual,
		padapter->recvpriv.noise
		);
				
	*eof = 1;
	return len;
}

int proc_set_rx_signal(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	u32 is_signal_dbg, signal_strength;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%u %u", &is_signal_dbg, &signal_strength);

		is_signal_dbg = is_signal_dbg==0?0:1;
		
		if(is_signal_dbg && num!=2)
			return count;
			
		signal_strength = signal_strength>100?100:signal_strength;
		signal_strength = signal_strength<0?0:signal_strength;

		padapter->recvpriv.is_signal_dbg = is_signal_dbg;
		padapter->recvpriv.signal_strength_dbg=signal_strength;

		if(is_signal_dbg)
			DBG_871X("set %s %u\n", "DBG_SIGNAL_STRENGTH", signal_strength);
		else
			DBG_871X("set %s\n", "HW_SIGNAL_STRENGTH");
		
	}
	
	return count;
	
}

int proc_get_ht_enable(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	
	int len = 0;
	
	if(pregpriv)
		len += snprintf(page + len, count - len,
			"%d\n",
			pregpriv->ht_enable
			);

	*eof = 1;
	return len;
}

int proc_set_ht_enable(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	char tmp[32];
	u32 mode;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d ", &mode);

		if( pregpriv && mode >= 0 && mode < 2 )
		{
			pregpriv->ht_enable= mode;
			printk("ht_enable=%d\n", pregpriv->ht_enable);
		}
	}
	
	return count;
	
}


int proc_get_cbw40_enable(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	
	int len = 0;
	
	if(pregpriv)
		len += snprintf(page + len, count - len,
			"%d\n",
			pregpriv->cbw40_enable
			);

	*eof = 1;
	return len;
}

int proc_set_cbw40_enable(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	char tmp[32];
	u32 mode;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d ", &mode);

		if( pregpriv && mode >= 0 && mode < 2 )
		{
			pregpriv->cbw40_enable= mode;
			printk("cbw40_enable=%d\n", mode);
		}
	}
	
	return count;
	
}

int proc_get_ampdu_enable(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	
	int len = 0;
	
	if(pregpriv)
		len += snprintf(page + len, count - len,
			"%d\n",
			pregpriv->ampdu_enable
			);

	*eof = 1;
	return len;
}

int proc_set_ampdu_enable(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	char tmp[32];
	u32 mode;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d ", &mode);

		if( pregpriv && mode >= 0 && mode < 3 )
		{
			pregpriv->ampdu_enable= mode;
			printk("ampdu_enable=%d\n", mode);
		}
	}
	
	return count;
	
}


int proc_get_two_path_rssi(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	
	int len = 0;
	
	if(padapter)
		len += snprintf(page + len, count - len,
			"%d %d\n",
			padapter->recvpriv.RxRssi[0],
			padapter->recvpriv.RxRssi[1]
			);

	*eof = 1;
	return len;
}

int proc_get_rx_stbc(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	
	int len = 0;
	
	if(pregpriv)
		len += snprintf(page + len, count - len,
			"%d\n",
			pregpriv->rx_stbc
			);

	*eof = 1;
	return len;
}

int proc_set_rx_stbc(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	char tmp[32];
	u32 mode;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d ", &mode);

		if( pregpriv && (mode == 0 || mode == 1|| mode == 2|| mode == 3))
		{
			pregpriv->rx_stbc= mode;
			printk("rx_stbc=%d\n", mode);
		}
	}
	
	return count;
	
}

int proc_get_vid(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	u16 VID=0;
	int len = 0;

	rtw_hal_get_hwreg(padapter, HW_VAR_VID, (u8 *)&VID);	
	len += snprintf(page + len, count - len,
		"%04x\n",
		VID
		);

	*eof = 1;
	return len;	
}

int proc_get_pid(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	u16 PID=0;	
	int len = 0;

	rtw_hal_get_hwreg(padapter, HW_VAR_PID, (u8 *)&PID);		
	len += snprintf(page + len, count - len,
		"%04x\n",
		PID
		);

	*eof = 1;
	return len;	
}

int proc_get_rssi_disp(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	*eof = 1;
	return 0;
}

int proc_set_rssi_disp(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	u32 enable=0;

	if (count < 1)
	{
		DBG_8192C("argument size is less than 1\n");
		return -EFAULT;
	}	

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%x", &enable);

		if (num !=  1) {
			DBG_8192C("invalid set_rssi_disp parameter!\n");
			return count;
		}
		
		if(enable)
		{			
			DBG_8192C("Turn On Rx RSSI Display Function\n");
			padapter->bRxRSSIDisplay = enable ;			
		}
		else
		{
			DBG_8192C("Turn Off Rx RSSI Display Function\n");
			padapter->bRxRSSIDisplay = 0 ;
		}
	
	}
	
	return count;
	
}	

		
#ifdef CONFIG_AP_MODE

int proc_get_all_sta_info(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	_irqL irqL;
	struct sta_info *psta;
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct sta_priv *pstapriv = &padapter->stapriv;
	int i, j;
	_list	*plist, *phead;
	struct recv_reorder_ctrl *preorder_ctrl;
	int len = 0;	
						

	len += snprintf(page + len, count - len, "sta_dz_bitmap=0x%x, tim_bitmap=0x%x\n", pstapriv->sta_dz_bitmap, pstapriv->tim_bitmap);
					
	_enter_critical_bh(&pstapriv->sta_hash_lock, &irqL);

	for(i=0; i< NUM_STA; i++)
	{
		phead = &(pstapriv->sta_hash[i]);
		plist = get_next(phead);
		
		while ((rtw_end_of_queue_search(phead, plist)) == _FALSE)
		{
			psta = LIST_CONTAINOR(plist, struct sta_info, hash_list);

			plist = get_next(plist);

			//if(extra_arg == psta->aid)
			{
				len += snprintf(page + len, count - len, "sta's macaddr:" MAC_FMT "\n", MAC_ARG(psta->hwaddr));
				len += snprintf(page + len, count - len, "rtsen=%d, cts2slef=%d\n", psta->rtsen, psta->cts2self);
				len += snprintf(page + len, count - len, "qos_en=%d, ht_en=%d, init_rate=%d\n", psta->qos_option, psta->htpriv.ht_option, psta->init_rate);	
				len += snprintf(page + len, count - len, "state=0x%x, aid=%d, macid=%d, raid=%d\n", psta->state, psta->aid, psta->mac_id, psta->raid);	
				len += snprintf(page + len, count - len, "bwmode=%d, ch_offset=%d, sgi=%d\n", psta->htpriv.bwmode, psta->htpriv.ch_offset, psta->htpriv.sgi);						
				len += snprintf(page + len, count - len, "ampdu_enable = %d\n", psta->htpriv.ampdu_enable);									
				len += snprintf(page + len, count - len, "agg_enable_bitmap=%x, candidate_tid_bitmap=%x\n", psta->htpriv.agg_enable_bitmap, psta->htpriv.candidate_tid_bitmap);
				len += snprintf(page + len, count - len, "sleepq_len=%d\n", psta->sleepq_len);
				len += snprintf(page + len, count - len, "capability=0x%x\n", psta->capability);
				len += snprintf(page + len, count - len, "flags=0x%x\n", psta->flags);
				len += snprintf(page + len, count - len, "wpa_psk=0x%x\n", psta->wpa_psk);
				len += snprintf(page + len, count - len, "wpa2_group_cipher=0x%x\n", psta->wpa2_group_cipher);
				len += snprintf(page + len, count - len, "wpa2_pairwise_cipher=0x%x\n", psta->wpa2_pairwise_cipher);
				len += snprintf(page + len, count - len, "qos_info=0x%x\n", psta->qos_info);
				len += snprintf(page + len, count - len, "dot118021XPrivacy=0x%x\n", psta->dot118021XPrivacy);
								
				for(j=0;j<16;j++)
				{							
					preorder_ctrl = &psta->recvreorder_ctrl[j];
					if(preorder_ctrl->enable)
					{
						len += snprintf(page + len, count - len, "tid=%d, indicate_seq=%d\n", j, preorder_ctrl->indicate_seq);
					}
				}		
									
			}							
			
		}
		
	}
	
	_exit_critical_bh(&pstapriv->sta_hash_lock, &irqL);

	*eof = 1;
	return len;

}
	
#endif		

#ifdef DBG_MEMORY_LEAK
#include <asm/atomic.h>
extern atomic_t _malloc_cnt;;
extern atomic_t _malloc_size;;

int proc_get_malloc_cnt(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	
	int len = 0;

	len += snprintf(page + len, count - len, "_malloc_cnt=%d\n", atomic_read(&_malloc_cnt));
	len += snprintf(page + len, count - len, "_malloc_size=%d\n", atomic_read(&_malloc_size));
				
	*eof = 1;
	return len;
}
#endif /* DBG_MEMORY_LEAK */

#ifdef CONFIG_FIND_BEST_CHANNEL
int proc_get_best_channel(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	int len = 0;
	u32 i, best_channel_24G = 1, best_channel_5G = 36, index_24G = 0, index_5G = 0;

	for (i=0; pmlmeext->channel_set[i].ChannelNum !=0; i++) {
		if ( pmlmeext->channel_set[i].ChannelNum == 1)
			index_24G = i;
		if ( pmlmeext->channel_set[i].ChannelNum == 36)
			index_5G = i;
	}	
	
	for (i=0; pmlmeext->channel_set[i].ChannelNum !=0; i++) {
		// 2.4G
		if ( pmlmeext->channel_set[i].ChannelNum == 6 ) {
			if ( pmlmeext->channel_set[i].rx_count < pmlmeext->channel_set[index_24G].rx_count ) {
				index_24G = i;
				best_channel_24G = pmlmeext->channel_set[i].ChannelNum;
			}
		}

		// 5G
		if ( pmlmeext->channel_set[i].ChannelNum >= 36
			&& pmlmeext->channel_set[i].ChannelNum < 140 ) {
			 // Find primary channel
			if ( (( pmlmeext->channel_set[i].ChannelNum - 36) % 8 == 0)
				&& (pmlmeext->channel_set[i].rx_count < pmlmeext->channel_set[index_5G].rx_count) ) {
				index_5G = i;
				best_channel_5G = pmlmeext->channel_set[i].ChannelNum;
			}
		}

		if ( pmlmeext->channel_set[i].ChannelNum >= 149
			&& pmlmeext->channel_set[i].ChannelNum < 165) {
			 // find primary channel
			if ( (( pmlmeext->channel_set[i].ChannelNum - 149) % 8 == 0)
				&& (pmlmeext->channel_set[i].rx_count < pmlmeext->channel_set[index_5G].rx_count) ) {
				index_5G = i;
				best_channel_5G = pmlmeext->channel_set[i].ChannelNum;
			}
		}
#if 1 // debug
		len += snprintf(page + len, count - len, "The rx cnt of channel %3d = %d\n", 
					pmlmeext->channel_set[i].ChannelNum, pmlmeext->channel_set[i].rx_count);
#endif
	}
	
	len += snprintf(page + len, count - len, "best_channel_5G = %d\n", best_channel_5G);
	len += snprintf(page + len, count - len, "best_channel_24G = %d\n", best_channel_24G);

	*eof = 1;
	return len;

}

int proc_set_best_channel(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	char tmp[32];

	if(count < 1)
		return -EFAULT;

	if(buffer && !copy_from_user(tmp, buffer, sizeof(tmp)))
	{
		int i;
		for(i = 0; pmlmeext->channel_set[i].ChannelNum != 0; i++)
		{
			pmlmeext->channel_set[i].rx_count = 0;
		}

		DBG_871X("set %s\n", "Clean Best Channel Count");
	}

	return count;
}
#endif /* CONFIG_FIND_BEST_CHANNEL */

#if defined(DBG_CONFIG_ERROR_DETECT)
#include <rtw_sreset.h>
int proc_get_sreset(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	int len = 0;
	
	*eof = 1;
	return len;
}

int proc_set_sreset(struct file *file, const char *buffer, unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	s32 trigger_point;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%d", &trigger_point);

		if (trigger_point == SRESET_TGP_NULL)
			rtw_hal_sreset_reset(padapter);
		else
			sreset_set_trigger_point(padapter, trigger_point);
	}
	
	return count;
	
}
#endif /* DBG_CONFIG_ERROR_DETECT */

#ifdef CONFIG_DM_ADAPTIVITY
int proc_get_dm_adaptivity(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	int len = 0;

	len += dm_adaptivity_get_parm_str(padapter, page, count);

	*eof = 1;
	return len;
}

int proc_set_dm_adaptivity(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	char tmp[32];
	u32 TH_L2H_ini;
	s8 TH_EDCCA_HL_diff;
	u32 IGI_Base;
	int ForceEDCCA;
	u8 AdapEn_RSSI;
	u8 IGI_LowerBound;

	if (count < 1)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {

		int num = sscanf(tmp, "%x %hhd %x %d %hhu %hhu",
			&TH_L2H_ini, &TH_EDCCA_HL_diff, &IGI_Base, &ForceEDCCA, &AdapEn_RSSI, &IGI_LowerBound);

		if (num != 6)
			return count;

		dm_adaptivity_set_parm(padapter, (s8)TH_L2H_ini, TH_EDCCA_HL_diff, (s8)IGI_Base, (bool)ForceEDCCA, AdapEn_RSSI, IGI_LowerBound);
	}
	
	return count;
}
#endif /* CONFIG_DM_ADAPTIVITY */

#endif

