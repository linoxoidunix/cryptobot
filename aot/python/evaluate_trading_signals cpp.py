import warnings
warnings.filterwarnings('ignore')

from time import time
from io import StringIO
import sys, os
import warnings
from pathlib import Path
import pandas as pd
import numpy as np
import statsmodels.api as sm
import matplotlib.pyplot as plt
import seaborn as sns

import lightgbm as lgb

from scipy.stats import spearmanr, pearsonr
from dateutil.parser import parse

from alphalens import plotting
from alphalens import performance as perf
from alphalens.utils import get_clean_factor_and_forward_returns, rate_of_return, std_conversion
from alphalens.tears import (create_summary_tear_sheet,
                             create_full_tear_sheet)

sys.path.insert(1, os.path.join(sys.path[0], '..'))
from utils import MultipleTimeSeriesCV

from dateutil.parser import parse

def is_date(string, fuzzy=False):
    """
    Return whether the string can be interpreted as a date.

    :param string: str, string to check for date
    :param fuzzy: bool, ignore unknown tokens in string if True
    """
    try: 
        parse(string, fuzzy=fuzzy)
        return True

    except ValueError:
        return False


sns.set_style('whitegrid')


# lgb_ic = lgb_ic.loc[lgb_ic['index'].str.isdigit()]


YEAR = 252
idx = pd.IndexSlice

scope_params = ['lookahead', 'train_length', 'test_length']
daily_ic_metrics = ['daily_ic_mean', 'daily_ic_mean_n', 'daily_ic_median', 'daily_ic_median_n']
lgb_train_params = ['learning_rate', 'num_leaves', 'feature_fraction', 'min_data_in_leaf']
catboost_train_params = ['max_depth', 'min_child_samples']

results_path = Path('results', 'BTCUSDT')
if not results_path.exists():
    results_path.mkdir(parents=True)

lgb_metrics = pd.DataFrame()

with pd.HDFStore(results_path / 'tuning_lgb.h5') as store:
    for i, key in enumerate(
        [k[1:] for k in store.keys() if k[1:].startswith('metrics')]):
        _, t, train_length, test_length = key.split('/')[:4]
        attrs = {
            'lookahead': t,
            'train_length': train_length,
            'test_length': test_length
        }
        s = store[key].to_dict()
        s.update(attrs)
        if i == 0:
            lgb_metrics = pd.Series(s).to_frame(i)
        else:
            lgb_metrics[i] = pd.Series(s)

id_vars = scope_params + lgb_train_params + daily_ic_metrics

lgb_metrics = pd.melt(lgb_metrics.T.drop('t', axis=1), 
                  id_vars=id_vars, 
                  value_name='ic', 
                  var_name='boost_rounds').dropna().apply(pd.to_numeric)

lgb_metrics.to_hdf('data/model_tuning.h5', 'lgb/metrics')

int_cols = ['lookahead', 'train_length', 'test_length', 'boost_rounds']

lgb_ic = []
with pd.HDFStore(results_path / 'tuning_lgb.h5') as store:
    keys = [k[1:] for k in store.keys()]
    for key in keys:
        _, t, train_length, test_length = key.split('/')[:4]
        if key.startswith('daily_ic'):
            df = (store[key]
                  .drop(['boosting', 'objective', 'verbose'], axis=1)
                 .assign(lookahead=t, 
                         train_length=train_length, 
                         test_length=test_length))
            lgb_ic.append(df)
    lgb_ic = pd.concat(lgb_ic).reset_index()

#print(lgb_ic.date.unique())
#clean lgb_ic from int boost_rounds, some string. stay onle data
# lgb_ic['index_is_date'] = lgb_ic['index'].apply(lambda x : is_date(str(x)))

# lgb_ic['index_is_not_int'] = lgb_ic['index'].apply(lambda x : (not str(x).isdigit()))
# lgb_ic = lgb_ic.loc[(lgb_ic['index_is_date'] == True) & (lgb_ic['index_is_not_int'] == True)]
# print(lgb_ic.info())
# lgb_ic.drop(['index_is_date', 'index_is_not_int'], axis = 1, inplace=True)
# print(lgb_ic.lookahead.unique())

#id_vars = scope_params + lgb_train_params
# column_without_number = [t for t in lgb_ic.columns if not str(t).isdigit() ]
# number_column = [item for item in lgb_ic.columns if item not in column_without_number]
# value_column = lgb_ic[number_column[1]]
# value_column.rename('ic', inplace=True)
# lgb_ic = lgb_ic.drop(number_column, axis = 1)
# lgb_ic = lgb_ic.loc[lgb_ic['index'].str.isdigit()]
# lgb_ic.rename(columns={'index':'boost_rounds'}, inplace=True)
# lgb_ic = pd.concat([lgb_ic, value_column], axis=1).reindex(lgb_ic.index)
# lgb_ic.lookahead = lgb_ic.lookahead.apply(lambda x : int(x))
# lgb_ic.ic = lgb_ic.ic.apply(lambda x : float(x))
# lgb_ic.boost_rounds = lgb_ic.boost_rounds.apply(lambda x : int(x))
# lgb_ic.train_length = lgb_ic.train_length.apply(lambda x : int(x))
# lgb_ic.test_length = lgb_ic.test_length.apply(lambda x : int(x))
#lgb_ic.rename(columns={'index':'date'}, inplace=True)
id_vars = ['date'] + scope_params + lgb_train_params
lgb_ic = pd.melt(lgb_ic, 
                 id_vars=id_vars, 
                 value_name='ic', 
                 var_name='boost_rounds').dropna()
#now columns lookahead, ic< boost_rounds, train_length, test_length is object
lgb_ic.lookahead = lgb_ic.lookahead.apply(lambda x : int(x))
lgb_ic.ic = lgb_ic.ic.apply(lambda x : float(x))
lgb_ic.boost_rounds = lgb_ic.boost_rounds.apply(lambda x : int(x))
lgb_ic.train_length = lgb_ic.train_length.apply(lambda x : int(x))
lgb_ic.test_length = lgb_ic.test_length.apply(lambda x : int(x))
# print(lgb_ic.lookahead.unique())


lgb_ic.to_hdf('data/model_tuning.h5', 'lgb/ic')
lgb_daily_ic = lgb_ic.groupby(
    id_vars[0:] +
    ['boost_rounds']).ic.mean().to_frame('ic').reset_index()
lgb_daily_ic.to_hdf('data/model_tuning.h5', 'lgb/daily_ic')

fig, axes = plt.subplots(ncols=2, figsize=(15, 5), sharey=True)
sns.boxenplot(x='lookahead', y='ic', hue='model',
              data=lgb_metrics.assign(model='lightgbm'), ax=axes[0])
axes[0].axhline(0, ls='--', lw=1, c='k')
axes[0].set_title('Overall IC')
sns.boxenplot(x='lookahead', y='ic', hue='model',
               data=lgb_daily_ic.assign(model='lightgbm'), ax=axes[1])
axes[1].axhline(0, ls='--', lw=1, c='k')
axes[1].set_title('Daily IC')
fig.tight_layout()
# print(lgb_ic.lookahead.unique())
#plt.show(block=True)
# Hyper parametr impact
lin_reg = {}
for t in [1, 21]:
    df_ = lgb_ic[lgb_ic.lookahead==t]
    y, X = df_.ic, df_.drop(['ic','date'], axis=1)
    X = sm.add_constant(pd.get_dummies(X, columns=X.columns, drop_first=True))
    model = sm.OLS(endog=y, exog=X.astype(float))
    lin_reg[t] = model.fit()
    s = lin_reg[t].summary()
    coefs = pd.read_csv(StringIO(s.tables[1].as_csv())).rename(columns=lambda x: x.strip())
    coefs.columns = ['variable', 'coef', 'std_err', 't', 'p_value', 'ci_low', 'ci_high']
    coefs.to_csv(f'results/linreg_result_{t:02}.csv', index=False)

def visualize_lr_result(model, ax):
    ci = model.conf_int()
    errors = ci[1].sub(ci[0]).div(2)

    coefs = (model.params.to_frame('coef').assign(error=errors)
             .reset_index().rename(columns={'index': 'variable'}))
    coefs = coefs[~coefs['variable'].str.startswith('date')&(coefs.variable!='const')]

    coefs.plot(x='variable', y='coef', kind='bar', 
                 ax=ax, color='none', capsize=3,
                 yerr='error', legend=False)
    ax.set_ylabel('IC')
    ax.set_xlabel('')
    ax.scatter(x=np.arange(len(coefs)), marker='_', s=120, y=coefs['coef'], color='black')
    ax.axhline(y=0, linestyle='--', color='black', linewidth=1)
    ax.xaxis.set_ticks_position('none')

fig, axes = plt.subplots(nrows=2, ncols=1, figsize=(20, 8), sharey=True)
axes = axes.flatten()
for i, t in enumerate([1,21]):
    visualize_lr_result(lin_reg[t], axes[i])
    axes[i].set_title(f'Lookahead: {t} Day(s)')
fig.suptitle('OLS Coefficients & Confidence Intervals', fontsize=20)
fig.tight_layout()
fig.subplots_adjust(top=.92);

group_cols = scope_params + lgb_train_params + ['boost_rounds']
x = lgb_daily_ic.groupby('lookahead', group_keys=False).apply(lambda x: x.nlargest(3, 'ic'))

lgb_metrics.groupby('lookahead', group_keys=False).apply(lambda x: x.nlargest(3, 'ic'))
lgb_metrics.groupby('lookahead', group_keys=False).apply(lambda x: x.nlargest(3, 'ic')).to_csv('results/best_lgb_model.csv', index=False)

lgb_metrics.groupby('lookahead', group_keys=False).apply(lambda x: x.nlargest(3, 'daily_ic_mean'))

sns.jointplot(x=lgb_metrics.daily_ic_mean,y=lgb_metrics.ic)

g = sns.catplot(x='lookahead', y='ic',
                col='train_length', row='test_length',
                data=lgb_metrics,
                kind='box')

t=1
g=sns.catplot(x='boost_rounds',
            y='ic',
            col='train_length',
            row='test_length',
            data=lgb_daily_ic[lgb_daily_ic.lookahead == t],
            kind='box')

#AlphaLens Analysis - Validation Performance
lgb_daily_ic = pd.read_hdf('data/model_tuning.h5', 'lgb/daily_ic')


def get_lgb_params(data, t=5, best=0):
    param_cols = scope_params[1:] + lgb_train_params + ['boost_rounds']
    df = data[data.lookahead==t].sort_values('ic', ascending=False).iloc[best]
    return df.loc[param_cols]

def get_lgb_key(t, p):
    key = f'{t}/{int(p.train_length)}/{int(p.test_length)}/{p.learning_rate}/'
    return key + f'{int(p.num_leaves)}/{p.feature_fraction}/{int(p.min_data_in_leaf)}'

best_params = get_lgb_params(lgb_daily_ic, t=1, best=0)
best_params.to_hdf('data/data.h5', 'best_params')

# print(best_params)

def select_ic(params, ic_data, lookahead):
    return ic_data.loc[(ic_data.lookahead == lookahead) &
                       (ic_data.train_length == params.train_length) &
                       (ic_data.test_length == params.test_length) &
                       (ic_data.learning_rate == params.learning_rate) &
                       (ic_data.num_leaves == params.num_leaves) &
                       (ic_data.feature_fraction == params.feature_fraction) &
                       (ic_data.boost_rounds == params.boost_rounds), ['date', 'ic']].set_index('date')

fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(20, 5))
axes = axes.flatten()
for i, t in enumerate([1, 21]):
    params = get_lgb_params(lgb_daily_ic, t=t)
    data = select_ic(params, lgb_ic, lookahead=t).sort_index()
    rolling = data.rolling(3).ic.mean().dropna()
    avg = data.ic.mean()
    med = data.ic.median()
    rolling.plot(ax=axes[i], title=f'Horizon: {t} Day(s) | IC: Mean={avg*100:.2f}   Median={med*100:.2f}')
    axes[i].axhline(avg, c='darkred', lw=1)
    axes[i].axhline(0, ls='--', c='k', lw=1)

fig.suptitle('3-day Rolling Information Coefficient', fontsize=16)
fig.tight_layout()
fig.subplots_adjust(top=0.92);

# print(lgb_daily_ic)
# Get Predictions for Validation Period

# We retrieve the predictions for the 10 validation runs:
lookahead = 1
topn = 10
for best in range(topn):
    best_params = get_lgb_params(lgb_daily_ic, t=lookahead, best=best)
    # print(best_params)
    key = get_lgb_key(lookahead, best_params)
    # print(key)
    rounds = str(int(best_params.boost_rounds))
    if best == 0:
        best_predictions = pd.read_hdf(results_path / 'tuning_lgb.h5', 'predictions/' + key)
        best_predictions = best_predictions[rounds].to_frame(best)
    else:
        best_predictions[best] = pd.read_hdf(results_path / 'tuning_lgb.h5',
                                             'predictions/' + key)[rounds]
best_predictions.index.names=['date']
best_predictions = best_predictions.sort_index()
best_predictions.to_hdf('data/predictions.h5', f'lgb/train/{lookahead:02}')
# best_predictions.info()
best_predictions['asset'] = 'BTCUSDT'
best_predictions.set_index(['asset'], append=True, inplace=True)
best_predictions = best_predictions.swaplevel()

# Get Trade Prices

# Using next available prices.

def get_trade_prices(tickers):
    prices = pd.read_csv('data/ohlcv.csv', sep='\t')
    # prices2 = pd.read_csv('data/ohlcv2.csv', sep='\t')
    frames = [prices]
    prices = pd.concat(frames).reset_index()
    prices['date'] = [parse(x, fuzzy=True) for x in prices['timestamp']]
    prices['asset']=tickers
    prices.set_index(['date'], inplace=True)
    prices.set_index(['asset'], append=True, inplace=True)
    prices.drop(['timestamp','index'], axis=1, inplace=True)
    #prices = prices[::4]
    return (prices['close']
            .unstack('asset')
            )

#close prices since 2021-01-01 for 2023-01-01
trade_prices = get_trade_prices('BTCUSDT')

# persist result in case we want to rerun:
trade_prices.to_hdf('data/model_tuning.h5', 'trade_prices/model_selection')
trade_prices = pd.read_hdf('data/model_tuning.h5', 'trade_prices/model_selection')

#We average the top five models and provide the corresponding prices to Alphalens, in order to compute the mean period-wise return earned on an equal-weighted portfolio invested in the daily factor quintiles for various holding periods:
factor = best_predictions.iloc[:, :5].mean(1).dropna().swaplevel()

# factor_data = get_clean_factor_and_forward_returns(factor=factor,
#                                                    # factor=factor[::4],
#                                                    prices=trade_prices,
#                                                    quantiles=5,
#                                                    periods=(1, 5, 10, 21))

# factor_quantile = factor_data.groupby(grouper)['factor'] \
#         .apply(quantile_calc, 5, None, False, True)

# print(factor_quantile)

# mean_quant_ret_bydate, std_quant_daily = perf.mean_return_by_quantile(
#     factor_data,
#     by_date=True,
#     by_group=False,
#     demeaned=True,
#     group_adjust=False,
# )

# factor_returns = perf.factor_returns(factor_data)

# mean_quant_ret, std_quantile = perf.mean_return_by_quantile(factor_data,
#                                                             by_group=False,
#                                                             demeaned=True)



# mean_quant_rateret = mean_quant_ret.apply(rate_of_return, axis=0,
#                                           base_period=mean_quant_ret.columns[0])

# mean_quant_ret_bydate, std_quant_daily = perf.mean_return_by_quantile(
#     factor_data,
#     by_date=True,
#     by_group=False,
#     demeaned=True,
#     group_adjust=False,
# )

# mean_quant_rateret_bydate = mean_quant_ret_bydate.apply(
#     rate_of_return,
#     base_period=mean_quant_ret_bydate.columns[0],
# )

# compstd_quant_daily = std_quant_daily.apply(std_conversion,
#                                             base_period=std_quant_daily.columns[0])

# alpha_beta = perf.factor_alpha_beta(factor_data,
#                                     demeaned=True)

# mean_ret_spread_quant, std_spread_quant = perf.compute_mean_returns_spread(
#     mean_quant_rateret_bydate,
#     factor_data["factor_quantile"].max(),
#     factor_data["factor_quantile"].min(),
#     std_err=compstd_quant_daily,
# )

# mean_ret_spread_quant.mean().mul(10000).to_frame('Mean Period Wise Spread (bps)').join(alpha_beta.T).T

# fig, axes = plt.subplots(ncols=3, figsize=(18, 4))


# plotting.plot_quantile_returns_bar(mean_quant_rateret, ax=axes[0])
# plt.setp(axes[0].xaxis.get_majorticklabels(), rotation=0)
# axes[0].set_xlabel('Quantile')

# plotting.plot_cumulative_returns_by_quantile(mean_quant_ret_bydate['1D'],
#                                              freq=pd.tseries.offsets.BDay(),
#                                              period='1D',
#                                              ax=axes[1])
# axes[1].set_title('Cumulative Return by Quantile (1D Period)')

# title = "Cumulative Return - Factor-Weighted Long/Short PF (1D Period)"
# plotting.plot_cumulative_returns(factor_returns['1D'],
#                                  period='1D',
#                                  freq=pd.tseries.offsets.BDay(),
#                                  title=title,
#                                  ax=axes[2])

# fig.suptitle('Alphalens - Validation Set Performance', fontsize=14)
# fig.tight_layout()
# fig.subplots_adjust(top=.85);
#---------------------------------------------------------------------------------
data = pd.read_hdf('data/data.h5', 'model_data')
data = data.set_index('ts_parsed').dropna().drop('timestamp', axis=1)
data.index.names = ['date']
labels = sorted(data.filter(like='fwd').columns)
features = data.columns.difference(labels).tolist()
categoricals = ['year', 'month', 'weekday']
param_names = ['learning_rate', 'num_leaves',
               'feature_fraction', 'min_data_in_leaf']
base_params = dict(boosting='gbdt',
                   objective='regression',
                   verbose=-1)
#     # iterate over (shuffled) hyperparameter combinations
#     for p, param_vals in enumerate(cv_params_):
#         key = f'{lookahead}/{train_length}/{test_length}/' + '/'.join([str(p) for p in param_vals])
#         params = dict(zip(param_names, param_vals))
#         params.update(base_params)
#         print(key)
#         start = time()
#         cv_preds, nrounds = [], []
#         ic_cv = defaultdict(list)
        
#         # iterate over folds
#         for i, (train_idx, test_idx) in enumerate(cv.split(X=outcome_data)):
            
#             # select train subset
#             lgb_train = lgb_data.subset(used_indices=train_idx.tolist(),
#                                        params=params).construct()
#             model = lgb.train(params=params,
#                               train_set=lgb_train,
#                               num_boost_round=num_boost_round)
#             xxx = model.feature_importance(importance_type='gain')

#             # log feature importance
#             if i == 0:
#                 fi = get_fi(model).to_frame()
#             else:
#                 fi[i] = get_fi(model)

#             # capture predictions
#             test_set = outcome_data.iloc[test_idx, :]
#             X_test = test_set.loc[:, model.feature_name()]
#             y_test = test_set.loc[:, label]
#             y_pred = {str(n): model.predict(X_test, num_iteration=n) for n in num_iterations}
            
#             # record predictions for each fold
#             cv_preds.append(y_test.to_frame('y_test').assign(**y_pred).assign(i=i))

#1) save first 3 best model for 21 lookahead 
lookahead = 21
topn = 3
#need_write_x - variable for writing X once 
need_write_x = True
results_path = Path('models')
lgb_store = Path(results_path /'features_df.h5')

for best in range(topn):
    best_params = get_lgb_params(lgb_daily_ic, t=lookahead, best=best)
   
    train_length = best_params.train_length
    test_length = best_params.test_length
    num_boost_round = best_params.boost_rounds
    # randomized grid search
    
    cv_params_ = [best_params.learning_rate, best_params.num_leaves, best_params.feature_fraction, best_params.min_data_in_leaf]

    # set up cross-validation
    n_splits = 1
    print(f'Lookahead: {lookahead:2.0f} | '
          f'Train: {train_length:3.0f} | '
          f'Test: {test_length:2.0f} | '
          f'Params: {len(cv_params_):3.0f} | ')
    
    cv = MultipleTimeSeriesCV(n_splits=n_splits,
                              lookahead=lookahead,
                              test_period_length=test_length,
                              train_period_length=train_length,
                              date_idx='date')
    label = 'r21_fwd'
    #select only 2021 year among 2021-2022. outcome_data index has delta 6 hours. for working alphalens need result only every day not intaday
    outcome_data = data.loc['2020-01-01':'2021-12-31', features + [label]].dropna()
    #print(outcome_data)
    lgb_data = lgb.Dataset(data=outcome_data.drop(label, axis=1),
                           label=outcome_data[label],
                           categorical_feature=categoricals,
                           free_raw_data=False)
    T = 0
    predictions, metrics, feature_importance, daily_ic = [], [], [], []
    params = dict(zip(param_names, cv_params_))
    params.update(base_params)
    print(key)
    start = time()
    cv_preds, nrounds = [], []

    # iterate over folds
    for train_idx in cv.split(X=outcome_data):
    # select train subset
        lgb_train = lgb_data.subset(used_indices=train_idx[0],
                                    params=params).construct()
        
        if(need_write_x):
            lgb_train.get_data().to_hdf(lgb_store, 'features')
            data.iloc[train_idx[0]].to_hdf(lgb_store, 'test_idx')
            #np.save(arr=train_idx[0], file='models/test_idx')
            need_write_x = False

        model = lgb.train(params=params,
                            train_set=lgb_train,
                            num_boost_round=num_boost_round)
        print(model.feature_name())
        model.save_model(f'models/model_{best}.txt')
       